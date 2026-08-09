#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// 定義 ESP32-S2 的自訂腳位
#define SDA_PIN 8
#define SCL_PIN 9
#define FSR_PIN 4

// 建立 MPU6050 物件
Adafruit_MPU6050 mpu;

void setup(void) {
  // 初始化序列埠，請將序列埠監控視窗的鮑率設為 115200
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // 等待序列埠連線
  }

  Serial.println("ESP32-S2 系統啟動...");

  // 初始化 I2C (指定 SDA 與 SCL 腳位)
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化 MPU6050
  if (!mpu.begin()) {
    Serial.println("找不到 MPU6050 晶片，請檢查接線是否正確！");
    while (1) {
      delay(10); // 找不到感測器就停在此處
    }
  }
  Serial.println("MPU6050 初始化成功！");

  // 設定 MPU6050 的測量範圍與濾波器 (可依需求調整)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // 加速度範圍：±8G
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);      // 陀螺儀範圍：±500 度/秒
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // 濾波器頻寬設定為 21 Hz
  
  // 設定 FSR 腳位為輸入模式 (雖然類比讀取預設為輸入，但寫上較為嚴謹)
  pinMode(FSR_PIN, INPUT);
  
  Serial.println("開始讀取感測器資料：");
  Serial.println("--------------------------------");
}

void loop() {
  // ================= 1. 讀取 FSR 壓力感測器 =================
  // ESP32-S2 的 ADC 預設解析度通常為 13-bit (0~8191)
  int fsrValue = analogRead(FSR_PIN);

  // ================= 2. 讀取 MPU6050 六軸感測器 =================
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // ================= 3. 將資料輸出到序列埠 =================
  // 列印壓力值
  Serial.print("FSR 壓力值: ");
  Serial.print(fsrValue);

  // 列印加速度 (單位: m/s^2)
  Serial.print("  |  Accel (X, Y, Z): ");
  Serial.print(a.acceleration.x); Serial.print(", ");
  Serial.print(a.acceleration.y); Serial.print(", ");
  Serial.print(a.acceleration.z);

  // 列印陀螺儀 (單位: rad/s)
  Serial.print("  |  Gyro (X, Y, Z): ");
  Serial.print(g.gyro.x); Serial.print(", ");
  Serial.print(g.gyro.y); Serial.print(", ");
  Serial.println(g.gyro.z);

  // 稍微延遲以符合 50Hz (20ms) 的傳輸間隔
  delay(20);
}