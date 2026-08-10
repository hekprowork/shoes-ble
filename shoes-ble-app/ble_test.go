package main

import (
	"encoding/binary"
	"math"
	"testing"
)

func TestParseSensorData(t *testing.T) {
	// Create mock 16 bytes for SensorData
	// 2 uint16 (4 bytes), 6 int16 (12 bytes) = 16 bytes
	buf := make([]byte, 16)
	binary.LittleEndian.PutUint16(buf[0:2], 100)  // FSR1
	binary.LittleEndian.PutUint16(buf[2:4], 200)  // FSR2
	binary.LittleEndian.PutUint16(buf[4:6], 1)    // AccX
	binary.LittleEndian.PutUint16(buf[6:8], 2)    // AccY
	binary.LittleEndian.PutUint16(buf[8:10], 3)   // AccZ
	binary.LittleEndian.PutUint16(buf[10:12], 4)  // GyroX
	binary.LittleEndian.PutUint16(buf[12:14], 5)  // GyroY
	binary.LittleEndian.PutUint16(buf[14:16], 6)  // GyroZ

	data, err := ParseSensorData(buf)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if data.FSR1 != 100 {
		t.Errorf("expected FSR1 100, got %d", data.FSR1)
	}
	if data.FSR2 != 200 {
		t.Errorf("expected FSR2 200, got %d", data.FSR2)
	}
	if data.Acc.X != 1 || data.Acc.Y != 2 || data.Acc.Z != 3 {
		t.Errorf("expected Acc {1, 2, 3}, got %+v", data.Acc)
	}
	if data.Gyro.X != 4 || data.Gyro.Y != 5 || data.Gyro.Z != 6 {
		t.Errorf("expected Gyro {4, 5, 6}, got %+v", data.Gyro)
	}

	// Test invalid length
	_, err = ParseSensorData(buf[:15])
	if err == nil {
		t.Error("expected error on invalid length, got nil")
	}
}

func TestParseQuaternionData(t *testing.T) {
	buf := make([]byte, 16)
	binary.LittleEndian.PutUint32(buf[0:4], math.Float32bits(1.0)) // W
	binary.LittleEndian.PutUint32(buf[4:8], math.Float32bits(0.0)) // X
	binary.LittleEndian.PutUint32(buf[8:12], math.Float32bits(0.5)) // Y
	binary.LittleEndian.PutUint32(buf[12:16], math.Float32bits(-0.5)) // Z

	data, err := ParseQuaternionData(buf)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if data.W != 1.0 {
		t.Errorf("expected W 1.0, got %f", data.W)
	}
	if data.X != 0.0 {
		t.Errorf("expected X 0.0, got %f", data.X)
	}
	if data.Y != 0.5 {
		t.Errorf("expected Y 0.5, got %f", data.Y)
	}
	if data.Z != -0.5 {
		t.Errorf("expected Z -0.5, got %f", data.Z)
	}

	// Test invalid length
	_, err = ParseQuaternionData(buf[:15])
	if err == nil {
		t.Error("expected error on invalid length, got nil")
	}
}
