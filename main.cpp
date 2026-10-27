#include <atomic>
#include <cstdlib>
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

#include "src/engine/IngestionEngine.h"

// ---------------------------------------------------------------------------
// UAV Real-Time Telemetry Ingestion Engine
//
// Listens on UDP_PORT for binary telemetry packets from a UAV simulator,
// decodes them into sensor state, and renders a live dashboard in the terminal.
//
// Usage:
//   ./uav_telemetry_ingester [port]      (default port: 14550)
//
// Run the simulator in a second terminal:
//   ./uav_simulator [port]
//
// Press Ctrl+C to stop gracefully.
// ---------------------------------------------------------------------------

// Atomic flag set by the signal handler — polled by the main thread.
// Using a flag keeps the signal handler async-signal-safe: it does nothing
// except store a bool, with no locks, allocations, or function calls.
static std::atomic<bool> shutdown_requested{ false };

static void signalHandler(int /*sig*/) {
    shutdown_requested.store(true);
}

int main(const int argc, char* argv[]) {
    uint16_t port = 14550;

    if (argc == 2) {
        char* end = nullptr;
        const long p = std::strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || p <= 0 || p > 65535) {
            std::cerr << "Invalid port: " << argv[1] << "\n";
            return 1;
        }
        port = static_cast<uint16_t>(p);
    }

    // Catch Ctrl+C for graceful shutdown
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        IngestionEngine engine(port);

        std::cout << "UAV Telemetry Ingester listening on UDP port " << port << "\n";
        std::cout << "Start the simulator: ./uav_simulator " << port << "\n\n";

        engine.start();

        // Block main thread until Ctrl+C — engine.stop() runs here on the
        // main thread where it is safe to join threads and acquire locks.
        while (!shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "\nShutdown signal caught safely. Joining threads and cleaning up...\n";
        engine.stop(); // Safe to join threads here on the main execution track!
        std::cout << "\nTelemetry ingestion stopped. Goodbye.\n";
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
