#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "src/display/TerminalRenderer.h"
#include "src/network/UdpReceiver.h"
#include "src/sensors/Battery.h"
#include "src/sensors/FlightController.h"
#include "src/sensors/Gps.h"
#include "src/sensors/Imu.h"

// ---------------------------------------------------------------------------
// IngestionEngine
//
// Owns all components and coordinates three concurrent activities:
//
//   Thread 1 (receiver_thread_) — UdpReceiver::run()
//     Blocks on select(), receives UDP packets, calls sensor update() callbacks.
//     Runs until stop() is called.
//
//   Thread 2 (render_thread_) — renderLoop()
//     Wakes every RENDER_INTERVAL_MS, takes state snapshots from all sensors,
//     checks alert thresholds, and calls TerminalRenderer::render().
//
//   Main thread — calls start(), polls g_shutdown_requested, then calls stop().
//     stop() joins both threads — safe to do on the main thread.
//
// Uses std::thread — manually joined in stop().
// ---------------------------------------------------------------------------

class IngestionEngine {
public:
    // All 4 sensors share a single UDP port — distinguished by SensorId byte
    explicit IngestionEngine(uint16_t port);
    ~IngestionEngine();

    // Start receiver and render threads — returns immediately.
    // The caller is responsible for blocking until shutdown is requested,
    // then calling stop().
    void start();

    // Stop all threads and join them. Must be called from the main thread —
    // not from a signal handler (joins are not async-signal-safe).
    void stop();

private:
    void renderLoop();
    void checkAlerts(const TelemetryState& telemetry_state);

    // Returns seconds since the sensor last sent a packet (-1.0 if never received)
    // Inline template implementation prevents header-linking and resolution compilation errors
    template<typename State>
    static double secondsSinceUpdate(const State& s) {
        using namespace std::chrono;
        if (s.stale)
            return -1.0;
        return duration_cast<duration<double>>(steady_clock::now() - s.last_update).count();
    }

    static constexpr int RENDER_INTERVAL_MS = 500; // render ~2 fps
    static constexpr double SENSOR_TIMEOUT_SEC = 3.0; // declare offline after this

    std::atomic<bool> running_{ false };
    std::thread receiver_thread_;
    std::thread render_thread_;
    UdpReceiver receiver_;

    FlightController fc_;
    Gps gps_;
    Imu imu_;
    Battery battery_;

    TerminalRenderer terminalRenderer_;
    std::chrono::steady_clock::time_point start_time_;

    // Track last alerted charge level to avoid alert spam
    float last_alerted_charge_{ 100.0f };
    // Track last GPS signal level that triggered an alert — avoids spam
    float last_alerted_gps_signal_{ 100.0f };

    // Per-sensor offline alert hysteresis — true once we've fired the alert,
    // reset when data resumes so we can fire again on the next dropout.
    bool fc_offline_alerted_{ false };
    bool gps_offline_alerted_{ false };
    bool imu_offline_alerted_{ false };
    bool batt_offline_alerted_{ false };
};
