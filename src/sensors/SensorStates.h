#pragma once

#include <chrono>
#include <cstdint>

// ---------------------------------------------------------------------------
// BaseSensorState
//
// Shared telemetry metadata container. Provides runtime snapshot tracking
// utilities without carrying dependency overhead or mutex locks.
// ---------------------------------------------------------------------------
struct BaseSensorState {
    bool stale{ true }; // true until first packet arrives
    std::chrono::steady_clock::time_point last_update{}; // wall-clock time of last packet

    // Pure functional connection monitoring bound directly to the snapshot instance
    [[nodiscard]] bool isOffline(const double timeout_sec = 3.0) const {
        if (stale)
            return false; // Still waiting for initial handshake — show "Waiting", not "Offline"

        using namespace std::chrono;
        return duration_cast<duration<double>>(steady_clock::now() - last_update).count() > timeout_sec;
    }
};

// ---------------------------------------------------------------------------
// Sensor State Snapshots (Stack allocated data copies used by rendering loops)
// ---------------------------------------------------------------------------

struct alignas(8) FlightControllerState : BaseSensorState {
    float altitude_m{ 0.0f };
    float vertical_speed_mps{ 0.0f };
    float horizontal_speed_mps{ 0.0f };
    uint64_t timestamp_ms{ 0 };
};

struct alignas(8) GpsState : BaseSensorState {
    double latitude{ 0.0 };
    double longitude{ 0.0 };
    float signal_strength{ 0.0f };
    uint64_t timestamp_ms{ 0 };
};

struct alignas(8) ImuState : BaseSensorState {
    float roll_deg{ 0.0f };
    float pitch_deg{ 0.0f };
    float yaw_deg{ 0.0f };
    uint64_t timestamp_ms{ 0 };
};

struct alignas(8) BatteryState : BaseSensorState {
    float charge_percentage{ 100.0f };
    float voltage{ 0.0f };
    float temperature_celsius{ 0.0f };
    uint64_t timestamp_ms{ 0 };
};
