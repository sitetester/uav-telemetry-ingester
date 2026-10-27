#include "./Battery.h"

void Battery::update(const BatteryPacket& pkt) {
    std::lock_guard lock(mutex_);

    BatteryState newState{};
    newState.stale = false;
    newState.last_update = std::chrono::steady_clock::now();
    newState.charge_percentage = pkt.charge_percentage;
    newState.voltage = pkt.voltage;
    newState.temperature_celsius = pkt.temperature_celsius;
    newState.timestamp_ms = pkt.timestamp_ms;

    state_ = newState;
}

BatteryState Battery::state() const {
    std::lock_guard lock(mutex_);
    return state_;
}

BatteryStatus Battery::statusFor(const float charge_percentage) {
    if (charge_percentage < 10.0f)
        return BatteryStatus::Critical;
    if (charge_percentage < 30.0f)
        return BatteryStatus::Warning;
    return BatteryStatus::Normal;
}
