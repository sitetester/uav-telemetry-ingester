#include "Telemetry.h"

#include <iostream>
#include <sys/socket.h>

#include <cerrno>
#include <cstring>
#include "SimulationState.h"
#include "src/packets/WirePackets.h"

template<typename T>
static void send_packet(const int fd, const sockaddr_in& dest, const T& pkt) {
    // Create a safe, temporary byte buffer directly on the stack
    std::uint8_t buffer[sizeof(T)];

    // Perform a safe bitwise copy into raw bytes to guarantee data alignment safety
    std::memcpy(buffer, &pkt, sizeof(T));

    // Dispatch the clean, copy-safe bytes over the UDP socket
    const ssize_t bytes_sent = sendto(fd,
        reinterpret_cast<const char*>(buffer),
        sizeof(T),
        0,
        reinterpret_cast<const sockaddr*>(&dest),
        sizeof(dest));

    // Validate output: UDP is all-or-nothing, so a simple negative check is ideal
    if (bytes_sent < 0) {
        std::cerr << "[SIMULATOR ERROR] Telemetry frame send failed: " << std::strerror(errno) << std::endl;
    }
}



void Telemetry::sendTelemetry(const std::chrono::steady_clock::time_point now) {
    using namespace std::chrono;

    // -- Flight Controller @ 10 Hz (every 100ms) --
    if (duration_cast<milliseconds>(now - fc_last_).count() >= 100) {
        updateAndSendFlightController();
        fc_last_ = now;
    }

    // -- GPS @ 5 Hz (every 200ms) --
    if (duration_cast<milliseconds>(now - gps_last_).count() >= 200) {
        updateAndSendGps();
        gps_last_ = now;
    }

    // -- IMU @ 50 Hz (every 20ms) --
    if (duration_cast<milliseconds>(now - imu_last_).count() >= 20) {
        updateAndSendImu();
        imu_last_ = now;
    }

    // -- Battery @ 1 Hz (every 1000ms) --
    if (duration_cast<milliseconds>(now - battery_last_).count() >= 1000) {
        updateAndSendBattery();
        battery_last_ = now;
    }
}

/**
 * @brief Thread-safe high-precision epoch clock utility.
 *
 * @return uint64_t Milliseconds elapsed since the Unix epoch.
 */
uint64_t Telemetry::nowMs() noexcept {
    using namespace std::chrono;

    // Capture the current system wall-clock point transaction safely
    const auto time_point_now = system_clock::now();

    // Cast the raw time point down to a stable millisecond duration track
    const auto duration_since_epoch = time_point_now.time_since_epoch();
    const auto duration_ms = duration_cast<milliseconds>(duration_since_epoch);

    // Extract the primitive integral scalar ticks
    return static_cast<uint64_t>(duration_ms.count());
}

void Telemetry::updateAndSendFlightController() {
    state_.updateFlightController();

    // 2. Dispatch the wire package instantly down the socket
    const FlightControllerPacket pkt{
        .sensor_type = SensorType::FlightController,
        .timestamp_ms = nowMs(),
        .altitude_m = state_.fc.altitude,
        .vertical_speed_mps = state_.fc.vertical_speed,
        .horizontal_speed_mps = state_.fc.horizontal_speed,
    };
    send_packet(fd_, dest_, pkt);
}

void Telemetry::updateAndSendGps() {
    state_.updateGps();

    const GpsPacket pkt{
        .sensor_type = SensorType::Gps,
        .timestamp_ms = nowMs(),
        .latitude = state_.gps.latitude,
        .longitude = state_.gps.longitude,
        .signal_strength = state_.gps.gps_signal,
    };
    send_packet(fd_, dest_, pkt);
}

void Telemetry::updateAndSendImu() {
    state_.updateImu();

    const ImuPacket pkt{
        .sensor_type = SensorType::Imu,
        .timestamp_ms = nowMs(),
        .roll_deg = state_.imu.roll,
        .pitch_deg = state_.imu.pitch,
        .yaw_deg = state_.imu.yaw,
    };
    send_packet(fd_, dest_, pkt);
}

void Telemetry::updateAndSendBattery() {
    state_.updateBattery();

    const BatteryPacket pkt{
        .sensor_type = SensorType::Battery,
        .timestamp_ms = nowMs(),
        .charge_percentage = state_.battery.charge,
        .voltage = state_.battery.voltage,
        .temperature_celsius = state_.battery.temperature_celsius,
    };
    send_packet(fd_, dest_, pkt);
}
