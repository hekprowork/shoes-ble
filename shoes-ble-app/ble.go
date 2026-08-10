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

type SensorData struct {
	FSR1  uint16 `json:"fsr1"`
	FSR2  uint16 `json:"fsr2"`
	AccX  int16  `json:"accX"`
	AccY  int16  `json:"accY"`
	AccZ  int16  `json:"accZ"`
	GyroX int16  `json:"gyroX"`
	GyroY int16  `json:"gyroY"`
	GyroZ int16  `json:"gyroZ"`
}

type QuaternionData struct {
	W float32 `json:"w"`
	X float32 `json:"x"`
	Y float32 `json:"y"`
	Z float32 `json:"z"`
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
				if len(buf) == 16 {
					var data SensorData
					reader := bytes.NewReader(buf)
					binary.Read(reader, binary.LittleEndian, &data.FSR1)
					binary.Read(reader, binary.LittleEndian, &data.FSR2)
					binary.Read(reader, binary.LittleEndian, &data.AccX)
					binary.Read(reader, binary.LittleEndian, &data.AccY)
					binary.Read(reader, binary.LittleEndian, &data.AccZ)
					binary.Read(reader, binary.LittleEndian, &data.GyroX)
					binary.Read(reader, binary.LittleEndian, &data.GyroY)
					binary.Read(reader, binary.LittleEndian, &data.GyroZ)
					runtime.EventsEmit(b.ctx, "sensor_data", data)
				}
			})
		} else if c.UUID() == quaternionCharUUID {
			c.EnableNotifications(func(buf []byte) {
				if len(buf) == 16 {
					var data QuaternionData
					reader := bytes.NewReader(buf)
					binary.Read(reader, binary.LittleEndian, &data.W)
					binary.Read(reader, binary.LittleEndian, &data.X)
					binary.Read(reader, binary.LittleEndian, &data.Y)
					binary.Read(reader, binary.LittleEndian, &data.Z)
					runtime.EventsEmit(b.ctx, "quaternion_data", data)
				}
			})
		}
	}
	runtime.EventsEmit(b.ctx, "ble_status", "Monitoring...")
}
