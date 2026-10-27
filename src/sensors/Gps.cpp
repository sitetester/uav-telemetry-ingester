#include "Gps.h"

void Gps::update(const GpsPacket& pkt) {
    std::lock_guard lock(mutex_);

    GpsState newState{};
    newState.stale = false;
    newState.last_update = std::chrono::steady_clock::now();
    newState.latitude = pkt.latitude;
    newState.longitude = pkt.longitude;
    newState.signal_strength = pkt.signal_strength;
    newState.timestamp_ms = pkt.timestamp_ms;

    state_ = newState;
}

GpsState Gps::state() const {
    std::lock_guard lock(mutex_);
    return state_;
}
