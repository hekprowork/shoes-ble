#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHRS_Madgwick.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SENSOR_SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define SENSOR_CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"
#define QUATERNION_CHARACTERISTIC_UUID "abcdef02-1234-5678-1234-56789abcdef0"

#define FSR1_PIN 0
#define FSR2_PIN 1
#define SDA_PIN 7
#define SCL_PIN 6

constexpr float FILTER_FREQUENCY_HZ = 50.0f;
constexpr float FILTER_GAIN = 0.033f;
constexpr float RADIANS_TO_DEGREES = 57.2957795f;
constexpr uint32_t SAMPLE_INTERVAL_US = 20000;
constexpr uint32_t MAX_FILTER_DT_US = 100000;
constexpr uint8_t SERIAL_LOG_DIVIDER = 10;

#pragma pack(push, 1)
struct SensorData {
  uint16_t fsr1;
  uint16_t fsr2;
  int16_t accX;
  int16_t accY;
  int16_t accZ;
  int16_t gyroX;
  int16_t gyroY;
  int16_t gyroZ;
};

struct QuaternionData {
  float w;
  float x;
  float y;
  float z;
};
#pragma pack(pop)

static_assert(sizeof(SensorData) == 16, "SensorData must fit one BLE notification");
static_assert(sizeof(QuaternionData) == 16, "QuaternionData must fit one BLE notification");

Adafruit_MPU6050 mpu;
Adafruit_Madgwick filter(FILTER_GAIN);

BLEServer *server = nullptr;
BLECharacteristic *sensorCharacteristic = nullptr;
BLECharacteristic *quaternionCharacteristic = nullptr;

volatile bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t lastSampleUs = 0;
uint8_t serialLogCounter = 0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    deviceConnected = true;
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer *server) override {
    deviceConnected = false;
    Serial.println("BLE client disconnected");
  }
};

void configureBle() {
  BLEDevice::init("ESP32-MPU6050-Tester");
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(SENSOR_SERVICE_UUID);
  sensorCharacteristic = service->createCharacteristic(
      SENSOR_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  sensorCharacteristic->addDescriptor(new BLE2902());

  quaternionCharacteristic = service->createCharacteristic(
      QUATERNION_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  quaternionCharacteristic->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising *advertising = server->getAdvertising();
  advertising->addServiceUUID(SENSOR_SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void updateConnectionState() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(100);
    server->startAdvertising();
    oldDeviceConnected = false;
    Serial.println("BLE advertising restarted");
  } else if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }
}

void publishSample(float dt) {
  const uint16_t fsr1 = static_cast<uint16_t>(analogRead(FSR1_PIN));
  const uint16_t fsr2 = static_cast<uint16_t>(4095 - analogRead(FSR2_PIN));

  sensors_event_t acceleration;
  sensors_event_t gyro;
  sensors_event_t temperature;
  mpu.getEvent(&acceleration, &gyro, &temperature);

  filter.updateIMU(
      gyro.gyro.x * RADIANS_TO_DEGREES,
      gyro.gyro.y * RADIANS_TO_DEGREES,
      gyro.gyro.z * RADIANS_TO_DEGREES,
      acceleration.acceleration.x,
      acceleration.acceleration.y,
      acceleration.acceleration.z,
      dt);

  SensorData sensorData = {
      fsr1,
      fsr2,
      static_cast<int16_t>(acceleration.acceleration.x * 100.0f),
      static_cast<int16_t>(acceleration.acceleration.y * 100.0f),
      static_cast<int16_t>(acceleration.acceleration.z * 100.0f),
      static_cast<int16_t>(gyro.gyro.x * 100.0f),
      static_cast<int16_t>(gyro.gyro.y * 100.0f),
      static_cast<int16_t>(gyro.gyro.z * 100.0f),
  };

  QuaternionData quaternionData;
  filter.getQuaternion(
      &quaternionData.w,
      &quaternionData.x,
      &quaternionData.y,
      &quaternionData.z);

  sensorCharacteristic->setValue(
      reinterpret_cast<uint8_t *>(&sensorData), sizeof(sensorData));
  quaternionCharacteristic->setValue(
      reinterpret_cast<uint8_t *>(&quaternionData), sizeof(quaternionData));

  if (deviceConnected) {
    sensorCharacteristic->notify();
    quaternionCharacteristic->notify();
  }

  serialLogCounter++;
  if (serialLogCounter >= SERIAL_LOG_DIVIDER) {
    serialLogCounter = 0;
    Serial.printf(
        "FSR: %u, %u | ACC: %.2f, %.2f, %.2f | GYRO: %.2f, %.2f, %.2f | "
        "Q: %.4f, %.4f, %.4f, %.4f\n",
        sensorData.fsr1,
        sensorData.fsr2,
        acceleration.acceleration.x,
        acceleration.acceleration.y,
        acceleration.acceleration.z,
        gyro.gyro.x,
        gyro.gyro.y,
        gyro.gyro.z,
        quaternionData.w,
        quaternionData.x,
        quaternionData.y,
        quaternionData.z);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(FSR1_PIN, INPUT);
  pinMode(FSR2_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!mpu.begin(0x68)) {
    Serial.println("MPU6050 initialization failed");
    while (true) {
      delay(100);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  filter.begin(FILTER_FREQUENCY_HZ);
  configureBle();

  lastSampleUs = micros();
  Serial.println("Smart Sole Sensor Node ready; Madgwick running at 50 Hz");
}

void loop() {
  updateConnectionState();

  const uint32_t nowUs = micros();
  const uint32_t elapsedUs = nowUs - lastSampleUs;
  if (elapsedUs < SAMPLE_INTERVAL_US) {
    delay(1);
    return;
  }

  lastSampleUs = nowUs;
  const float dt = elapsedUs > MAX_FILTER_DT_US
                       ? 1.0f / FILTER_FREQUENCY_HZ
                       : elapsedUs / 1000000.0f;
  publishSample(dt);
}
