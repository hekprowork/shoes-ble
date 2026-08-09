# 3. BLE Background Thread Architecture

Date: 2026-08-09

## Status

Accepted

## Context

The Python application uses `PySide6` for the GUI, which has its own event loop (`QEventLoop`). The Bluetooth library `bleak` relies on Python's `asyncio`. We need a reliable way to integrate these two event loops. High-frequency sensor data (IMU and pressure) is coming in, and the UI will perform demanding tasks like real-time charting and 3D rendering.

## Decision

We will run the `bleak` BLE client in a separate **background `QThread`** with its own `asyncio` event loop. Data received from the BLE device will be sent to the main GUI thread using **Qt Signals**.

## Consequences

- **Pros**: 
  - Prevents the UI rendering (which can block the main thread momentarily) from causing BLE packet drops or timeouts.
  - Clear separation of concerns (networking vs presentation).
  - Qt Signals provide thread-safe communication automatically.
- **Cons**:
  - Requires more boilerplate code to set up `QThread` and manage the lifecycle of the `asyncio` loop running inside it.
