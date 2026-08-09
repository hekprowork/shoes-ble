import numpy as np
from ahrs.filters import Madgwick

class SensorFusion:
    def __init__(self, sample_period=0.02):
        # sample_period is in seconds, e.g., 0.02 for 50Hz
        # Madgwick filter instance
        self.madgwick = Madgwick(sampleperiod=sample_period, gain=0.033)
        
        # Initial quaternion [w, x, y, z]
        self.q = np.array([1.0, 0.0, 0.0, 0.0])

    def update(self, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z):
        """
        Update the filter with new sensor data.
        acc in m/s^2
        gyro in rad/s
        Returns the updated quaternion as [w, x, y, z]
        """
        acc = np.array([acc_x, acc_y, acc_z])
        gyro = np.array([gyro_x, gyro_y, gyro_z])
        
        # Update the quaternion
        self.q = self.madgwick.updateIMU(self.q, gyr=gyro, acc=acc)
        return self.q
