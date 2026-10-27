#pragma once

#include <mutex>
#include "src/packets/WirePackets.h"
#include "src/sensors/SensorStates.h"

// ---------------------------------------------------------------------------
// Battery
//
// Holds the most recent decoded state from BatteryPacket.
// Also exposes alert threshold helpers used by TerminalRenderer.
// ---------------------------------------------------------------------------

enum class BatteryStatus {
    Normal, // charge >= 30%
    Warning, // charge 10–29%
    Critical, // charge < 10%
};

class Battery {
public:
    void update(const BatteryPacket& pkt);
    BatteryState state() const;

    // Derive status from current charge — pure, no lock needed
    static BatteryStatus statusFor(float charge_percentage);

private:
    mutable std::mutex mutex_;
    BatteryState state_;
};
