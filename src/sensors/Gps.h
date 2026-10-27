#pragma once

#include <mutex>
#include "src/packets/WirePackets.h"
#include "src/sensors/SensorStates.h"

class Gps {
public:
    // Called by the network thread (UdpReceiver), which writes new bytes into state_ every time  a packet arrives over
    // the socket.
    void update(const GpsPacket& pkt);
    GpsState state() const;

private:
    // std::mutex::lock() is a non-const operation, so in a const member function you can't call it unless the mutex is
    // mutable. mutable allows a member to be modified in const methods.
    mutable std::mutex mutex_;
    GpsState state_;
};
