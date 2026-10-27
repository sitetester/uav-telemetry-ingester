#pragma once

#include <algorithm>
#include <random>

struct FlightControllerSimState {
    float altitude = 50.0f;
    float vertical_speed = 0.5f;
    float horizontal_speed = 8.0f;
};

struct GpsSimState {
    double latitude = 54.6872;
    double longitude = 25.2797;
    float gps_signal = 85.0f;
    std::uniform_real_distribution<double> gps_drift{ -0.00001, 0.00001 };
};

struct ImuSimState {
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 90.0f;
};

struct BatterySimState {
    float charge = 95.0f;
    float voltage = 16.8f;
    float temperature_celsius = 25.0f;
};

// ===========================================================================
// SimulationState
// Master aggregate structure. Manages a single shared pseudo-random core
// engine to completely eliminate stack memory duplication and bloat.
// ===========================================================================
struct SimulationState {
    std::mt19937 rng{ std::random_device{}() };
    std::normal_distribution<float> noise{ 0.0f, 0.05f };

    FlightControllerSimState fc;
    GpsSimState gps;
    ImuSimState imu;
    BatterySimState battery;

    void updateFlightController() {
        const float delta_altitude = (fc.vertical_speed * 0.1f) + noise(rng);
        const float raw_vertical_speed = fc.vertical_speed + (noise(rng) * 0.5f);
        const float raw_horizontal_speed = fc.horizontal_speed + (noise(rng) * 0.2f);

        fc.altitude += delta_altitude;
        fc.vertical_speed = std::clamp(raw_vertical_speed, -5.0f, 5.0f);
        fc.horizontal_speed = std::clamp(raw_horizontal_speed, 0.0f, 20.0f);

        if (fc.altitude > 150.0f) {
            fc.vertical_speed = -0.3f;
        }
        if (fc.altitude < 10.0f) {
            fc.vertical_speed = 0.3f;
        }
    }

    void updateGps() {
        gps.latitude += gps.gps_drift(rng);
        gps.longitude += gps.gps_drift(rng);

        // Broken out clamp variables for signal tracking thresholds
        const float raw_gps_signal = gps.gps_signal + noise(rng) * 2.0f;
        gps.gps_signal = std::clamp(raw_gps_signal, 20.0f, 100.0f);
    }

    void updateImu() {
        // 1. Calculate the progression additions directly
        imu.roll += noise(rng) * 0.5f;
        imu.pitch += noise(rng) * 0.5f;
        imu.yaw += noise(rng) * 0.2f;

        // 2. Apply the protective caps directly to the values
        imu.roll = std::clamp(imu.roll, -45.0f, 45.0f);
        imu.pitch = std::clamp(imu.pitch, -45.0f, 45.0f);

        if (imu.yaw > 360.0f)
            imu.yaw -= 360.0f;
        if (imu.yaw < 0.0f)
            imu.yaw += 360.0f;
    }

    void updateBattery() {
        battery.charge -= 0.5f;
        if (battery.charge <= 15.0f) {
            battery.charge = 95.0f;
        }
        battery.voltage = 12.0f + (battery.charge / 100.0f) * 4.8f;

        // Broken out clamp variables for thermal environment properties
        const float raw_temperature = battery.temperature_celsius + (noise(rng) * 0.3f);
        battery.temperature_celsius = std::clamp(raw_temperature, 20.0f, 60.0f);
    }
};
