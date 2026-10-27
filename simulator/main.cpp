#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <random>
#include <string_view>
#include <thread>

// POSIX socket headers
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Telemetry.h"

static std::atomic<bool> running{ true };

static void signalHandler(int /*sig*/) {
    running.store(false);
}

static int createSocket(const char* host, const uint16_t port, sockaddr_in& dest) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error("Simulator: failed to create socket");
    }

    std::memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &dest.sin_addr) <= 0) {
        close(fd);
        throw std::runtime_error("Simulator: invalid IP address format");
    }
    return fd;
}

int main(const int argc, char* argv[]) {
    uint16_t port = 14550;
    if (argc == 2) {
        const std::string_view port_str = argv[1];

        // Use uint32_t to avoid compiler overload bugs and securely catch overflow values
        uint32_t parsed_port = 0;

        const auto [ptr, ec] = std::from_chars(port_str.data(), port_str.data() + port_str.size(), parsed_port);

        // Explicitly validate the numerical bounds for a network port
        if (ec == std::errc{} && ptr == port_str.data() + port_str.size() && parsed_port > 0 && parsed_port <= 65535) {
            port = static_cast<uint16_t>(parsed_port);
        } else {
            std::cerr << "Warning: Invalid port \"" << port_str << "\". Falling back to default " << port << ".\n";
        }
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    static constexpr auto DEFAULT_HOST = "127.0.0.1";
    sockaddr_in dest{};
    int fd;
    try {
        fd = createSocket(DEFAULT_HOST, port, dest);
    } catch (const std::exception& e) {
        std::cerr << "Fatal Initialization Error: " << e.what() << "\n";
        return -1;
    }

    std::cout << "UAV Simulator sending to " << DEFAULT_HOST << ":" << port << "\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    using namespace std::chrono;
    Telemetry telemetry{ fd, dest };
    while (running.load()) {
        const auto now = steady_clock::now();
        telemetry.sendTelemetry(now);
        std::this_thread::sleep_for(milliseconds(5));
    }

    close(fd);
    std::cout << "\nSimulator stopped.\n";
    return 0;
}
