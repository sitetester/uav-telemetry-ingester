#include "Imu.h"

void Imu::update(const ImuPacket& pkt) {
    std::lock_guard lock(mutex_);

    ImuState newState{};
    newState.stale = false;
    newState.last_update = std::chrono::steady_clock::now();
    newState.roll_deg = pkt.roll_deg;
    newState.pitch_deg = pkt.pitch_deg;
    newState.yaw_deg = pkt.yaw_deg;
    newState.timestamp_ms = pkt.timestamp_ms;

    state_ = newState;
}

ImuState Imu::state() const {
    std::lock_guard lock(mutex_);
    return state_;
}
