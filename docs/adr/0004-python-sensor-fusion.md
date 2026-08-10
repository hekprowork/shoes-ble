# 4. Sensor Fusion in Python

Date: 2026-08-09

## Status

Superseded by ADR-0005

## Context

The BLE payload from the ESP32-C3 only transmits raw accelerometer and gyroscope data (along with FSR data) in a 16-byte binary struct. It does not transmit quaternions. However, the application requires displaying the 3D posture, which requires quaternions.

## Decision

We will calculate the quaternions on the Python host side using a Sensor Fusion algorithm (e.g., Madgwick filter via the `ahrs` Python library). The ESP32-C3 firmware will remain unchanged and continue to send only raw data.

## Consequences

- **Pros**: Keeps the ESP32-C3 firmware simple and power-efficient. Keeps the BLE payload small (16 bytes), avoiding MTU limits. Takes advantage of the host machine's vastly superior computational power.
- **Cons**: Adds a computational step to the Python application's data processing pipeline, requiring additional dependencies (`ahrs`, `numpy`).
