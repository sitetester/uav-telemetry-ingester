#pragma once

#include <mutex>
#include "src/packets/WirePackets.h"
#include "src/sensors/SensorStates.h"

class FlightController {
public:
    // Called from UdpReceiver thread
    void update(const FlightControllerPacket& pkt);

    // Called from render thread — returns a copy (lock held briefly)
    FlightControllerState state() const;

private:
    mutable std::mutex mutex_;
    FlightControllerState state_;
};
