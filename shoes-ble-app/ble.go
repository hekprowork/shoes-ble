package main

import (
	"bytes"
	"context"
	"encoding/binary"
	"fmt"

	"github.com/wailsapp/wails/v2/pkg/runtime"
	"tinygo.org/x/bluetooth"
)

var adapter = bluetooth.DefaultAdapter

var (
	sensorServiceUUID, _  = bluetooth.ParseUUID("12345678-1234-5678-1234-56789abcdef0")
	sensorCharUUID, _     = bluetooth.ParseUUID("abcdef01-1234-5678-1234-56789abcdef0")
	quaternionCharUUID, _ = bluetooth.ParseUUID("abcdef02-1234-5678-1234-56789abcdef0")
)

type Vector3 struct {
	X int16 `json:"x"`
	Y int16 `json:"y"`
	Z int16 `json:"z"`
}

type SensorData struct {
	FSR1 uint16  `json:"fsr1"`
	FSR2 uint16  `json:"fsr2"`
	Acc  Vector3 `json:"acc"`
	Gyro Vector3 `json:"gyro"`
}

type QuaternionData struct {
	W float32 `json:"w"`
	X float32 `json:"x"`
	Y float32 `json:"y"`
	Z float32 `json:"z"`
}

func ParseSensorData(buf []byte) (SensorData, error) {
	var data SensorData
	if len(buf) != 16 {
		return data, fmt.Errorf("invalid sensor data length: %d", len(buf))
	}
	reader := bytes.NewReader(buf)
	err := binary.Read(reader, binary.LittleEndian, &data)
	return data, err
}

func ParseQuaternionData(buf []byte) (QuaternionData, error) {
	var data QuaternionData
	if len(buf) != 16 {
		return data, fmt.Errorf("invalid quaternion data length: %d", len(buf))
	}
	reader := bytes.NewReader(buf)
	err := binary.Read(reader, binary.LittleEndian, &data)
	return data, err
}

type BLEManager struct {
	ctx context.Context
}

func NewBLEManager(ctx context.Context) *BLEManager {
	return &BLEManager{ctx: ctx}
}

func (b *BLEManager) Start() {
	if err := adapter.Enable(); err != nil {
		runtime.EventsEmit(b.ctx, "ble_status", fmt.Sprintf("Failed to enable adapter: %v", err))
		return
	}

	runtime.EventsEmit(b.ctx, "ble_status", "Scanning for ESP32-MPU6050-Tester...")

	err := adapter.Scan(func(adapter *bluetooth.Adapter, device bluetooth.ScanResult) {
		if device.LocalName() == "ESP32-MPU6050-Tester" {
			adapter.StopScan()
			runtime.EventsEmit(b.ctx, "ble_status", "Found device, connecting...")
			go b.connect(device.Address)
		}
	})

	if err != nil {
		runtime.EventsEmit(b.ctx, "ble_status", fmt.Sprintf("Scan error: %v", err))
	}
}

func (b *BLEManager) connect(addr bluetooth.Address) {
	device, err := adapter.Connect(addr, bluetooth.ConnectionParams{})
	if err != nil {
		runtime.EventsEmit(b.ctx, "ble_status", fmt.Sprintf("Connect error: %v", err))
		return
	}
	runtime.EventsEmit(b.ctx, "ble_status", "Connected! Discovering services...")

	services, err := device.DiscoverServices([]bluetooth.UUID{sensorServiceUUID})
	if err != nil || len(services) == 0 {
		runtime.EventsEmit(b.ctx, "ble_status", "Failed to discover service")
		return
	}

	srv := services[0]
	chars, err := srv.DiscoverCharacteristics([]bluetooth.UUID{sensorCharUUID, quaternionCharUUID})
	if err != nil {
		runtime.EventsEmit(b.ctx, "ble_status", "Failed to discover characteristics")
		return
	}

	runtime.EventsEmit(b.ctx, "ble_status", "Subscribing to notifications...")

	for _, char := range chars {
		c := char // capture loop variable
		if c.UUID() == sensorCharUUID {
			c.EnableNotifications(func(buf []byte) {
				if data, err := ParseSensorData(buf); err == nil {
					runtime.EventsEmit(b.ctx, "sensor_data", data)
				}
			})
		} else if c.UUID() == quaternionCharUUID {
			c.EnableNotifications(func(buf []byte) {
				if data, err := ParseQuaternionData(buf); err == nil {
					runtime.EventsEmit(b.ctx, "quaternion_data", data)
				}
			})
		}
	}
	runtime.EventsEmit(b.ctx, "ble_status", "Monitoring...")
}
