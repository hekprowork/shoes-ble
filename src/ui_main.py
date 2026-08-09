import numpy as np
import pyqtgraph as pg
import pyqtgraph.opengl as gl
from PySide6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QPushButton
)
from PySide6.QtGui import QQuaternion, QVector3D, QMatrix4x4

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Smart Sole Sensor Node - BLE Receiver")
        self.resize(1200, 800)

        # Main Layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)

        # Left Panel (Charts)
        left_layout = QVBoxLayout()
        main_layout.addLayout(left_layout, stretch=2)

        # FSR Plot
        self.fsr_plot = pg.PlotWidget(title="FSR Pressure Sensors")
        self.fsr_plot.setYRange(0, 4095)
        self.fsr_plot.addLegend()
        self.fsr1_line = self.fsr_plot.plot(pen='r', name="FSR 1")
        self.fsr2_line = self.fsr_plot.plot(pen='b', name="FSR 2")
        left_layout.addWidget(self.fsr_plot)

        # IMU Plot
        self.imu_plot = pg.PlotWidget(title="IMU (Acc & Gyro)")
        self.imu_plot.addLegend()
        self.acc_x_line = self.imu_plot.plot(pen='r', name="Acc X")
        self.acc_y_line = self.imu_plot.plot(pen='g', name="Acc Y")
        self.acc_z_line = self.imu_plot.plot(pen='b', name="Acc Z")
        self.gyro_x_line = self.imu_plot.plot(pen='y', name="Gyro X")
        self.gyro_y_line = self.imu_plot.plot(pen='c', name="Gyro Y")
        self.gyro_z_line = self.imu_plot.plot(pen='m', name="Gyro Z")
        left_layout.addWidget(self.imu_plot)

        # Right Panel (3D Posture & Status)
        right_layout = QVBoxLayout()
        main_layout.addLayout(right_layout, stretch=1)

        # Status Label
        self.status_label = QLabel("Status: Disconnected")
        right_layout.addWidget(self.status_label)
        
        # 3D View
        self.gl_view = gl.GLViewWidget()
        self.gl_view.setCameraPosition(distance=15)
        right_layout.addWidget(self.gl_view)
        
        # Add a grid to 3D view
        grid = gl.GLGridItem()
        self.gl_view.addItem(grid)
        
        # Add a box representing the shoe/sensor
        # Size: 2 (width) x 4 (length) x 0.5 (height)
        self.box = gl.GLBoxItem(size=QVector3D(2, 4, 0.5), color=(0, 255, 0, 100))
        # Center the box
        self.box.translate(-1, -2, -0.25)
        self.gl_view.addItem(self.box)

        # Data buffers
        self.data_size = 500
        self.fsr1_data = np.zeros(self.data_size)
        self.fsr2_data = np.zeros(self.data_size)
        self.acc_x_data = np.zeros(self.data_size)
        self.acc_y_data = np.zeros(self.data_size)
        self.acc_z_data = np.zeros(self.data_size)
        self.gyro_x_data = np.zeros(self.data_size)
        self.gyro_y_data = np.zeros(self.data_size)
        self.gyro_z_data = np.zeros(self.data_size)

    def update_status(self, status: str):
        self.status_label.setText(f"Status: {status}")

    def update_sensor_data(self, fsr1, fsr2, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z, q):
        # Shift data left
        self.fsr1_data[:-1] = self.fsr1_data[1:]
        self.fsr2_data[:-1] = self.fsr2_data[1:]
        self.acc_x_data[:-1] = self.acc_x_data[1:]
        self.acc_y_data[:-1] = self.acc_y_data[1:]
        self.acc_z_data[:-1] = self.acc_z_data[1:]
        self.gyro_x_data[:-1] = self.gyro_x_data[1:]
        self.gyro_y_data[:-1] = self.gyro_y_data[1:]
        self.gyro_z_data[:-1] = self.gyro_z_data[1:]

        # Append new data
        self.fsr1_data[-1] = fsr1
        self.fsr2_data[-1] = fsr2
        self.acc_x_data[-1] = acc_x
        self.acc_y_data[-1] = acc_y
        self.acc_z_data[-1] = acc_z
        self.gyro_x_data[-1] = gyro_x
        self.gyro_y_data[-1] = gyro_y
        self.gyro_z_data[-1] = gyro_z

        # Update plots
        self.fsr1_line.setData(self.fsr1_data)
        self.fsr2_line.setData(self.fsr2_data)
        self.acc_x_line.setData(self.acc_x_data)
        self.acc_y_line.setData(self.acc_y_data)
        self.acc_z_line.setData(self.acc_z_data)
        self.gyro_x_line.setData(self.gyro_x_data)
        self.gyro_y_line.setData(self.gyro_y_data)
        self.gyro_z_line.setData(self.gyro_z_data)

        # Update 3D Posture
        # ahrs returns [w, x, y, z]
        w, x, y, z = q[0], q[1], q[2], q[3]
        quat = QQuaternion(w, x, y, z)
        
        # We need to apply the rotation to the box.
        # GLBoxItem has a transform property.
        transform = QMatrix4x4()
        transform.rotate(quat)
        
        # Translation to keep it centered
        transform.translate(-1, -2, -0.25)
        
        self.box.setTransform(transform)
