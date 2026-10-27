#pragma once

#include <mutex>
#include "src/packets/WirePackets.h"
#include "src/sensors/SensorStates.h"

class Imu {
public:
    void update(const ImuPacket& pkt);
    ImuState state() const;

private:
    mutable std::mutex mutex_;
    ImuState state_;
};
