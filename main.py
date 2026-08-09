import sys
import asyncio
import qasync
from PySide6.QtWidgets import QApplication
from src.ui_main import MainWindow
from src.ble_worker import BleWorker
from src.sensor_fusion import SensorFusion

class AppController:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.window = MainWindow()
        self.sensor_fusion = SensorFusion(sample_period=0.2) # Adjusted to 5Hz (200ms)
        
        self.ble_worker = BleWorker()
        
        # Connect signals
        self.ble_worker.signals.connection_status.connect(self.window.update_status)
        self.ble_worker.signals.data_received.connect(self.handle_data)
        
    def handle_data(self, fsr1, fsr2, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z):
        # Update quaternion using sensor fusion
        q = self.sensor_fusion.update(acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z)
        
        # Update UI
        self.window.update_sensor_data(
            fsr1, fsr2, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z, q
        )

    def run(self):
        self.window.show()
        
        loop = qasync.QEventLoop(self.app)
        asyncio.set_event_loop(loop)
        
        # Spawn the BLE background task in the shared event loop
        loop.create_task(self.ble_worker.start())
        
        with loop:
            loop.run_forever()
            
        # Cleanup
        self.ble_worker.stop()

if __name__ == "__main__":
    controller = AppController()
    controller.run()
