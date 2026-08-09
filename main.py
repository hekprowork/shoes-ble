import sys
import asyncio
import qasync
from PySide6.QtWidgets import QApplication
from src.ui_main import MainWindow
from src.ble_worker import BleWorker

class AppController:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.window = MainWindow()

        self.ble_worker = BleWorker()
        
        # Connect signals
        self.ble_worker.signals.connection_status.connect(self.window.update_status)
        self.ble_worker.signals.data_received.connect(
            self.window.update_sensor_data
        )
        self.ble_worker.signals.quaternion_received.connect(
            self.window.update_orientation
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
