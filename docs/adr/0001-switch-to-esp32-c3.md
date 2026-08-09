# Switch to ESP32-C3

We switched the microcontroller platform from the classic ESP32 (`esp32doit-devkit-v1`) to the ESP32-C3 (`esp32-c3-devkitm-1`).
This was necessary because the project requires assigning I2C pins to GPIO 7 (SDA) and GPIO 6 (SCL). On the classic ESP32, GPIO 6 through 11 are permanently reserved for the internal SPI flash memory, and using them causes boot failures or crashes. The ESP32-C3 architecture allows using these pins for I2C safely. As a result, the `platformio.ini` environment has been updated to target the ESP32-C3.
