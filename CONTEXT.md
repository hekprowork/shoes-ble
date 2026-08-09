# Ubiquitous Language / Glossary

- **Smart Sole Sensor Node (智能鞋底感測模組)**: The formal name of the ESP32-C3 based hardware and firmware node that collects FSR and MPU6050 data and broadcasts it over BLE.
- **FSR (Force Sensitive Resistor)**: Used to detect foot pressure. The system uses two FSRs.
- **MPU6050**: 6-axis IMU providing Acceleration (ACC) and Gyroscope (GYRO) data.
- **BLE Binary Payload**: The 16-byte packed binary struct transmitted over Bluetooth, containing raw sensor values to fit within a single 20-byte BLE MTU.
- **Transmission Interval (傳輸間隔)**: The frequency at which the ESP32-C3 sends the BLE payload. Set to 20ms (50Hz) to ensure smooth quaternion calculation via Sensor Fusion without overwhelming BLE bandwidth.
