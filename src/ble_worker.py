import asyncio
import struct
from bleak import BleakClient, BleakScanner
from PySide6.QtCore import QThread, Signal, QObject

SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
CHAR_UUID = "abcdef01-1234-5678-1234-56789abcdef0"

class BleSignals(QObject):
    # Signals must be defined on a QObject
    # data: fsr1, fsr2, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z
    data_received = Signal(float, float, float, float, float, float, float, float)
    connection_status = Signal(str)

class BleWorker(QThread):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.signals = BleSignals()
        self._is_running = True
        self.client = None

    def stop(self):
        self._is_running = False
        self.quit()
        self.wait()

    def run(self):
        # Create a new event loop for this thread
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        
        try:
            loop.run_until_complete(self.ble_task())
        except Exception as e:
            self.signals.connection_status.emit(f"Error: {e}")
        finally:
            loop.close()

    async def ble_task(self):
        import sys
        self.signals.connection_status.emit("Scanning for ESP32-C3...")
        print("Starting BLE scan... (this will take 10 seconds)", flush=True)
        
        # In a robust application, you'd scan for the device name or UUID.
        # Let's scan by service UUID.
        devices = await BleakScanner.discover(timeout=5.0)

        target_device = None

        for device in devices:
            print(f"Found: {device.name}  {device.address}")

            if device.name == DEVICE_NAME:
                target_device = device

        if target_device is None:
            print("\n找不到 ESP32-MPU6050-Tester")
            return

        print("\n找到 ESP32!")
        print(f"Name:    {target_device.name}")
        print(f"Address: {target_device.address}")
        
        if not target_device:
            self.signals.connection_status.emit("Device not found")
            return

        self.signals.connection_status.emit(f"Connecting to {target_device.name}...")
        
        async with BleakClient(target_device) as client:
            self.client = client
            if not client.is_connected:
                self.signals.connection_status.emit("Failed to connect")
                return
            
            self.signals.connection_status.emit("Connected")
            
            await client.start_notify(CHAR_UUID, self.notification_handler)
            
            # Keep the loop running
            while self._is_running and client.is_connected:
                await asyncio.sleep(0.1)
                
            if client.is_connected:
                await client.stop_notify(CHAR_UUID)

    def notification_handler(self, sender, data):
        if len(data) == 16:
            # Parse 16 bytes: 2x uint16, 6x int16 (Little Endian)
            unpacked = struct.unpack('<HHhhhhhh', data)
            fsr1 = unpacked[0]
            fsr2 = unpacked[1]
            acc_x = unpacked[2] / 100.0
            acc_y = unpacked[3] / 100.0
            acc_z = unpacked[4] / 100.0
            gyro_x = unpacked[5] / 100.0
            gyro_y = unpacked[6] / 100.0
            gyro_z = unpacked[7] / 100.0
            
            self.signals.data_received.emit(
                fsr1, fsr2, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z
            )
