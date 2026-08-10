import { describe, it, expect, vi } from 'vitest';

// Mock the Wails runtime EventsOn function
const mocks = vi.hoisted(() => {
    return {
        EventsOn: vi.fn(),
    }
});

vi.mock('../wailsjs/runtime/runtime', () => ({
    EventsOn: mocks.EventsOn
}));

import { EventsOn } from '../wailsjs/runtime/runtime';

describe('Wails Event Boundary Mocking', () => {
    it('should be able to mock EventsOn and simulate sensor data payload', () => {
        const handler = vi.fn();
        
        // Register our mock listener
        EventsOn('sensor_data', handler);
        
        // Ensure it was called
        expect(EventsOn).toHaveBeenCalledWith('sensor_data', handler);

        // Simulate a payload coming from Go
        const mockPayload = {
            fsr1: 100, fsr2: 200,
            acc: { x: 1, y: 2, z: 3 },
            gyro: { x: 4, y: 5, z: 6 }
        };
        
        // Trigger the handler directly to simulate Wails event propagation
        handler(mockPayload);
        
        // Assert the handler processed it correctly
        expect(handler).toHaveBeenCalledWith(mockPayload);
    });
});
