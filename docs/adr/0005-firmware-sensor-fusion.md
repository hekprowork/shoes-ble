# 5. Sensor Fusion on the Smart Sole Sensor Node

Date: 2026-08-10

## Status

Accepted

## Context

ADR-0004 placed Madgwick sensor fusion in the Python application. The filter
then ran at the BLE notification rate, and host scheduling added latency and
jitter. The Smart Sole Sensor Node can run the filter at the sensor sampling
rate and publish its orientation directly.

## Decision

The ESP32-C3 firmware will run the Madgwick IMU filter at 50 Hz. The existing
16-byte raw sensor characteristic remains unchanged. A second 16-byte BLE
characteristic publishes the quaternion as four little-endian `float` values
in `[w, x, y, z]` order. The Python application displays the firmware-provided
quaternion and no longer performs sensor fusion.

## Consequences

- Orientation updates use the sensor's sampling cadence rather than host event
  scheduling.
- Existing raw-data consumers remain compatible.
- Two BLE notifications are sent per sample while staying within the default
  BLE payload limit.
- The firmware gains an Adafruit AHRS dependency and additional computation.
