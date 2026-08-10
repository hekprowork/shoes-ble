package main

import (
	"context"
	"fmt"
)

// App struct
type App struct {
	ctx context.Context
	ble *BLEManager
}

// NewApp creates a new App application struct
func NewApp() *App {
	return &App{}
}

// startup is called when the app starts. The context is saved
// so we can call the runtime methods
func (a *App) startup(ctx context.Context) {
	a.ctx = ctx
	a.ble = NewBLEManager(ctx)
}

// StartBLE starts the BLE scanning and connection process
func (a *App) StartBLE() {
	if a.ble != nil {
		a.ble.Start()
	}
}

// Greet returns a greeting for the given name
func (a *App) Greet(name string) string {
	return fmt.Sprintf("Hello %s, It's show time!", name)
}
