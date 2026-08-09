#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SENSOR_SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"

// 定義 ESP32-DevKit V1 的 ADC 腳位
#define FSR1_PIN 0
#define FSR2_PIN 1

int fsr1Value , fsr2Value;
char payload[100]; // 用於序列埠顯示

// 定義一個 16 Bytes 的二進位結構體，確保能塞進 BLE 預設的 20 Bytes MTU 內
#pragma pack(push, 1)
struct SensorData {
  uint16_t fsr1;
  uint16_t fsr2;
  int16_t accX;   // 乘上 100 後的數值
  int16_t accY;
  int16_t accZ;
  int16_t gyroX;  // 乘上 100 後的數值
  int16_t gyroY;
  int16_t gyroZ;
};
#pragma pack(pop)
SensorData sensorData;

// 建立 MPU6050 物件
Adafruit_MPU6050 mpu;

// 將 BLEServer 宣告為全域變數，方便後續重新啟動廣播
BLEServer *pServer = NULL;
BLECharacteristic *pSensorCharacteristic = NULL;

// 新增：連線狀態追蹤變數
bool deviceConnected = false;
bool oldDeviceConnected = false;

// 伺服器回呼函式處理連線與斷線
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("✅ 有裝置連線了！");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("❌ 裝置斷線了！");
  }
};

void setup(void) {
  // 初始化序列埠 (請將序列埠監控視窗設為 115200)
  Serial.begin(115200);
  delay(1000); // 給序列埠一點緩衝時間
  Serial.println("\nESP32 系統啟動，準備初始化感測器...");
  
  // 初始化 BLE
  BLEDevice::init("ESP32-MPU6050-Tester");
  pServer = BLEDevice::createServer(); // 使用全域變數 pServer
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pSensorService = pServer->createService(SENSOR_SERVICE_UUID);
  pSensorCharacteristic = pSensorService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pSensorCharacteristic->addDescriptor(new BLE2902());
  pSensorService->start();
  
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SENSOR_SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  BLEDevice::startAdvertising();
  Serial.println("📡 藍牙廣播已啟動，等待連線...");

  // 設定 FSR 腳位為輸入
  pinMode(FSR1_PIN, INPUT);
  pinMode(FSR2_PIN, INPUT);

  // 設定 I2C 腳位: SDA=7, SCL=6
  Wire.begin(7, 6);

  // 初始化 MPU6050 (指定 I2C 位址為 0x68)
  if (!mpu.begin(0x68)) {
    Serial.println("❌ 找不到 MPU6050！請檢查接線：SDA->7, SCL->6，或確認位址是否為 0x68");
    while (1) {
      delay(100); // 找不到就卡在這裡
    }
  }
  Serial.println("✅ MPU6050 初始化成功！");

  // 設定 MPU6050 參數
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  Serial.println("--------------------------------");
  Serial.println("開始讀取感測器資料：");
}

void loop() {
  // 1. 讀取 FSR 壓力值
  fsr1Value = analogRead(FSR1_PIN);
  fsr2Value = 4095 - analogRead(FSR2_PIN); // 反轉 FSR2 的值

  // 2. 讀取 MPU6050 資料
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // 3. 輸出到序列埠並格式化 payload
  snprintf(payload, sizeof(payload), "FSR1: %d, FSR2: %d | ACC: %.2f, %.2f, %.2f | GYRO: %.2f, %.2f, %.2f\n",
                fsr1Value, fsr2Value,
                a.acceleration.x, a.acceleration.y, a.acceleration.z,
                g.gyro.x, g.gyro.y, g.gyro.z);
  Serial.print(payload);

  // 4. 打包成 16 bytes 二進位結構體
  sensorData.fsr1 = (uint16_t)fsr1Value;
  sensorData.fsr2 = (uint16_t)fsr2Value;
  sensorData.accX = (int16_t)(a.acceleration.x * 100);
  sensorData.accY = (int16_t)(a.acceleration.y * 100);
  sensorData.accZ = (int16_t)(a.acceleration.z * 100);
  sensorData.gyroX = (int16_t)(g.gyro.x * 100);
  sensorData.gyroY = (int16_t)(g.gyro.y * 100);
  sensorData.gyroZ = (int16_t)(g.gyro.z * 100);

  // --- BLE 狀態管理與發送資料 ---

  // 狀況 A：有裝置連線時，才發送資料 (Notify)
  if (deviceConnected) {
    pSensorCharacteristic->setValue((uint8_t*)&sensorData, sizeof(sensorData));
    pSensorCharacteristic->notify();
  }

  // 狀況 B：裝置剛剛斷線 (從 連線 -> 斷線)
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 稍微等待一下，讓藍牙堆疊完成斷線作業
    pServer->startAdvertising(); // 重新啟動廣播
    Serial.println("🔄 重新啟動藍牙廣播，等待新連線...");
    oldDeviceConnected = deviceConnected; // 更新狀態紀錄
  }

  // 狀況 C：裝置剛剛連線 (從 斷線 -> 連線)
  if (deviceConnected && !oldDeviceConnected) {
    // 可以在這裡做一些剛連線時的初始化
    oldDeviceConnected = deviceConnected; // 更新狀態紀錄
  }

  delay(200); // 稍微延遲以方便觀看
}