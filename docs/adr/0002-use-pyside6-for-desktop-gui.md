# 2. Use PySide6 for Desktop GUI

Date: 2026-08-09

## Status

Accepted

## Context

We need a way to display real-time sensor data (FSR and IMU) and a 3D visualization of the posture (using quaternions) received from an ESP32-C3 via Bluetooth (BLE) in a Python project managed by Pixi. We considered a Web Dashboard (FastAPI + Three.js), a Desktop GUI, and a Rapid Prototype (VPython).

## Decision

We will build a **Desktop GUI using PySide6**. To handle the data visualization, we will use `pyqtgraph` for the real-time sensor charts, and `PyOpenGL` embedded in PySide6 to render the 3D posture.

## Consequences

- **Pros**: High performance for real-time data plotting and 3D rendering. Native desktop window experience. No need to manage a web server and websockets.
- **Cons**: Requires integrating `asyncio` (used by the `bleak` BLE library) with the Qt event loop, which adds complexity to threading and event handling.
