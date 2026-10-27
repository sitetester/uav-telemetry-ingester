#include "UdpReceiver.h"

#include <cstddef>
#include <stdexcept>
#include <string>

// POSIX socket headers (macOS / Linux)
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "src/packets/WirePackets.h"

UdpReceiver::UdpReceiver(const uint16_t port) :
    port_(port) {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        throw std::runtime_error("UdpReceiver: failed to create socket");
    }

    // Allow quick restart without "Address already in use"
    constexpr int opt = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sockfd_);
        throw std::runtime_error("UdpReceiver: bind failed on port " + std::to_string(port_));
    }
}

UdpReceiver::~UdpReceiver() {
    stop();

    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
}

void UdpReceiver::run() {
    running_.store(true);

    // Receive buffer — sized to the largest possible packet
    char buf[MAX_PACKET_SIZE * 2]; // extra headroom for safety

    while (running_.load()) {
        // C++17 designated initialization style — re-allocated and reset cleanly each loop pass
        timeval timeout{
            .tv_sec = 0,
            .tv_usec = 100'000 // 100 ms
        };

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd_, &read_fds);

        if (const int ready = select(sockfd_ + 1, &read_fds, nullptr, nullptr, &timeout); ready <= 0) {
            // Timeout or signal — loop back and check running_
            continue;
        }

        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        const std::ptrdiff_t len =
            recvfrom(sockfd_, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&sender), &sender_len);

        if (len <= 0) {
            continue;
        }

        processIncomingPacket(buf, len);
    }
}

void UdpReceiver::stop() {
    running_.store(false);
}

// ---------------------------------------------------------------------------
// Inspect the first byte (SensorId) and reinterpret_cast the
// buffer into the matching packet struct, then invoke the registered callback.
// ---------------------------------------------------------------------------
void UdpReceiver::processIncomingPacket(const char* buf, const std::ptrdiff_t len) {
    if (len < 1)
        return;

    bool success = false;
    switch (static_cast<SensorType>(static_cast<uint8_t>(buf[0]))) {
        case SensorType::FlightController:
            success = parseAndDispatch<FlightControllerPacket>(buf, len, fc_cb_);
            break;

        case SensorType::Gps:
            success = parseAndDispatch<GpsPacket>(buf, len, gps_cb_);
            break;

        case SensorType::Imu:
            success = parseAndDispatch<ImuPacket>(buf, len, imu_cb_);
            break;

        case SensorType::Battery:
            success = parseAndDispatch<BatteryPacket>(buf, len, battery_cb_);
            break;

        default:
            ++packets_dropped_;
            return;
    }

    if (success) {
        ++packets_received_;
    }
}
