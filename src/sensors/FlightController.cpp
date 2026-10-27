#include "FlightController.h"

void FlightController::update(const FlightControllerPacket& pkt) {
    std::lock_guard lock(mutex_);

    FlightControllerState newState{};
    newState.stale = false;
    newState.last_update = std::chrono::steady_clock::now();
    newState.altitude_m = pkt.altitude_m;
    newState.vertical_speed_mps = pkt.vertical_speed_mps;
    newState.horizontal_speed_mps = pkt.horizontal_speed_mps;
    newState.timestamp_ms = pkt.timestamp_ms;

    state_ = newState;
}

FlightControllerState FlightController::state() const {
    std::lock_guard lock(mutex_);
    return state_;
}
