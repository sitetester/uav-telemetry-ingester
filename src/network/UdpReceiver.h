#pragma once

#include <atomic>
#include <functional>
#include <cstring> // Added to guarantee std::memcpy compiles everywhere

#include "src/packets/WirePackets.h"
#include "src/sensors/SensorStates.h"

// ---------------------------------------------------------------------------
// UdpReceiver
//
// Opens a UDP socket on the given port and continuously reads incoming packets
// in a tight loop (called from a dedicated std::thread in IngestionEngine).
//
// Each received buffer is inspected at byte 0 (the SensorId) and dispatched
// to the matching callback. The caller (IngestionEngine) registers one
// callback per sensor type.
//
// Raw bytes are reinterpret_cast to the appropriate packet struct — zero-copy,
// no serialization library needed. This mirrors how real embedded GCS software
// (e.g. MAVLink) handles binary telemetry frames.
// ---------------------------------------------------------------------------

class UdpReceiver {
public:
    // Callback signature type aliases — one per sensor type
    using FlightControllerCb = std::function<void(const FlightControllerPacket&)>;
    using GpsCb = std::function<void(const GpsPacket&)>;
    using ImuCb = std::function<void(const ImuPacket&)>;
    using BatteryCb = std::function<void(const BatteryPacket&)>;

    explicit UdpReceiver(uint16_t port);
    ~UdpReceiver();

    // Register callbacks before calling run()
    void onFlightController(const FlightControllerCb& cb) {
        fc_cb_ = cb;
    }
    void onGps(const GpsCb& cb) {
        gps_cb_ = cb;
    }
    void onImu(const ImuCb& cb) {
        imu_cb_ = cb;
    }
    void onBattery(const BatteryCb& cb) {
        battery_cb_ = cb;
    }

    // Start receiving — blocks until stop() is called (run on a dedicated thread)
    void run();

    // Signal run() to exit cleanly
    void stop();

    // Counters exposed for the status bar
    uint64_t packetsReceived() const {
        return packets_received_.load();
    }
    uint64_t packetsDropped() const {
        return packets_dropped_.load();
    }

private:
    void processIncomingPacket(const char* buf, std::ptrdiff_t len);

    // Dedicated template parser method replacing the lambda completely
    template<typename PacketType, typename CallbackType>
    bool parseAndDispatch(const char* buf, const std::ptrdiff_t len, const CallbackType& callback) {
        // Boundary size validation checks
        if (static_cast<std::size_t>(len) < sizeof(PacketType)) {
            ++packets_dropped_;
            return false;
        }

        if (callback) {
            // Instantiate a safe stack layout frame object instance
            PacketType pkt;

            // Extract memory bytes bitwise to eliminate strict aliasing undefined exceptions
            std::memcpy(&pkt, buf, sizeof(PacketType));

            // Pass the safe structure forward to the registered consumer handler
            callback(pkt);
        }
        return true;
    }
    uint16_t port_;
    int sockfd_{ -1 };

    FlightControllerCb fc_cb_;
    GpsCb gps_cb_;
    ImuCb imu_cb_;
    BatteryCb battery_cb_;

    std::atomic<bool> running_{ false };
    std::atomic<uint64_t> packets_received_{ 0 };
    std::atomic<uint64_t> packets_dropped_{ 0 };
};
