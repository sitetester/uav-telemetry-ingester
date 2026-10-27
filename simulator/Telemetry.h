#pragma once

#include <chrono>
#include <netinet/in.h>

#include "SimulationState.h"

// ---------------------------------------------------------------------------
// Telemetry
// Coordinates time-delta intervals and executes low-level UDP socket writes.
// ---------------------------------------------------------------------------
class Telemetry {
public:
    Telemetry(int fd, const sockaddr_in& dest) :
        fd_(fd),
        dest_(dest) {
    }

    // Direct interface to check scheduling intervals and pump packet frames
    void sendTelemetry(std::chrono::steady_clock::time_point now);

private:
    // Core Network Handlers
    int fd_;
    sockaddr_in dest_;
    SimulationState state_;

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimePoint fc_last_ = Clock::now();
    TimePoint gps_last_ = Clock::now();
    TimePoint imu_last_ = Clock::now();
    TimePoint battery_last_ = Clock::now();

    /**
    * Why static ?
    • - Intent/Readability: It instantly signals to us and the compiler that this function is a pure utility that
        won't touch internal class states.
    * - Zero Overhead: The compiler doesn't have to pass the implicit, hidden `this` pointer to the function, making
        the call slightly faster.
    */
    static uint64_t nowMs() noexcept;

    void updateAndSendFlightController();
    void updateAndSendGps();
    void updateAndSendImu();
    void updateAndSendBattery();
};
