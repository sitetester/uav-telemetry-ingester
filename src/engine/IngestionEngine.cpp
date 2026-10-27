#include "IngestionEngine.h"

#include <chrono>
#include <thread>
#include "src/display/panels/BatteryPanel.h"
#include "src/display/panels/FlightControllerPanel.h"
#include "src/display/panels/GpsPanel.h"
#include "src/display/panels/ImuPanel.h"

IngestionEngine::IngestionEngine(const uint16_t port) :
    receiver_(port) {
    // Register UdpReceiver callbacks — each lambda captures the sensor by ref
    // and calls update() from the receiver thread.
    receiver_.onFlightController([this](const FlightControllerPacket& pkt) { fc_.update(pkt); });
    receiver_.onGps([this](const GpsPacket& pkt) { gps_.update(pkt); });
    receiver_.onImu([this](const ImuPacket& pkt) { imu_.update(pkt); });
    receiver_.onBattery([this](const BatteryPacket& pkt) { battery_.update(pkt); });

    terminalRenderer_.addPanel(std::make_unique<FlightControllerPanel>(fc_));
    terminalRenderer_.addPanel(std::make_unique<GpsPanel>(gps_));
    terminalRenderer_.addPanel(std::make_unique<ImuPanel>(imu_));
    terminalRenderer_.addPanel(std::make_unique<BatteryPanel>(battery_));
}

void IngestionEngine::start() {
    running_.store(true);
    start_time_ = std::chrono::steady_clock::now();

    // UDP receive loop
    receiver_thread_ = std::thread([this] { receiver_.run(); });

    // render loop
    render_thread_ = std::thread([this] { renderLoop(); });
}

void IngestionEngine::stop() {
    running_.store(false);
    receiver_.stop();

    // Join threads explicitly (std::thread requires this before destruction)
    if (receiver_thread_.joinable())
        receiver_thread_.join();
    if (render_thread_.joinable())
        render_thread_.join();
}

// ---------------------------------------------------------------------------
// renderLoop — runs on render_thread_, wakes every RENDER_INTERVAL_MS
// ---------------------------------------------------------------------------
void IngestionEngine::renderLoop() {
    while (running_.load()) {
        TelemetryState telemetry_state = {
            .fc = fc_.state(),
            .gps = gps_.state(),
            .imu = imu_.state(),
            .batteryState = battery_.state(),
        };

        // Check thresholds and push alerts before rendering
        checkAlerts(telemetry_state);

        terminalRenderer_.render( //
            receiver_.packetsReceived(),
            receiver_.packetsDropped(),
            start_time_);

        std::this_thread::sleep_for(std::chrono::milliseconds(RENDER_INTERVAL_MS));
    }
}

// ---------------------------------------------------------------------------
// checkAlerts — push to renderer when sensor values cross thresholds.
// Uses hysteresis flags to avoid flooding the alert log.
// ---------------------------------------------------------------------------
void IngestionEngine::checkAlerts(const TelemetryState& telemetry_state) {
    // --- Offline detection ---------------------------------------------------
    // Fire once when a sensor goes silent; reset flag when data resumes.

    auto checkOffline =
        [&](const bool never_received, const double age_sec, bool& alerted_flag, const std::string& sensor_name) {
            if (never_received)
                return; // never had data — show "Waiting", not "Offline"
            if (age_sec > SENSOR_TIMEOUT_SEC) {
                if (!alerted_flag) {
                    const auto msg = sensor_name + " sensor offline";
                    terminalRenderer_.addAlert(AlertLevel::Warning, msg);
                    alerted_flag = true;
                }
            } else {
                alerted_flag = false; // data resumed — allow re-alerting on next dropout
            }
        };

    auto [fc, gps, imu, batteryState] = telemetry_state;

    checkOffline(fc.stale, secondsSinceUpdate(fc), fc_offline_alerted_, "Flight Controller");
    checkOffline(gps.stale, secondsSinceUpdate(gps), gps_offline_alerted_, "GPS");
    checkOffline(imu.stale, secondsSinceUpdate(imu), imu_offline_alerted_, "IMU");
    checkOffline(batteryState.stale, secondsSinceUpdate(batteryState), batt_offline_alerted_, "Battery");

    // --- Battery threshold alerts --------------------------------------------
    if (batteryState.stale)
        return;

    static constexpr float BATTERY_CRITICAL_PCT = 10.0f; // Threshold for critical battery landing alerts
    static constexpr float BATTERY_LOW_PCT = 30.0f; // Threshold for standard warning return alerts
    static constexpr float GPS_WEAK_SIGNAL_PCT = 30.0f; // Cutoff boundary where GPS tracking state degrades


    // Only fire each threshold alert once as charge drops through it
    if (const float charge = batteryState.charge_percentage;
        charge < BATTERY_CRITICAL_PCT && last_alerted_charge_ >= BATTERY_CRITICAL_PCT) {
        const auto msg = "Battery charge critical (" + std::to_string(static_cast<int>(charge)) + "%)";
        terminalRenderer_.addAlert(AlertLevel::Critical, msg);
        last_alerted_charge_ = charge;
    } else if (charge < BATTERY_LOW_PCT && last_alerted_charge_ >= BATTERY_LOW_PCT) {
        const auto msg = "Battery charge low (" + std::to_string(static_cast<int>(charge)) + "%)";
        terminalRenderer_.addAlert(AlertLevel::Warning, msg);
        last_alerted_charge_ = charge;
    }

    // --- GPS signal alert ----------------------------------------------------
    if (!gps.stale) {
        if (gps.signal_strength < GPS_WEAK_SIGNAL_PCT) {
            if (last_alerted_gps_signal_ >= GPS_WEAK_SIGNAL_PCT) {
                const auto msg = "GPS signal weak (" + std::to_string(static_cast<int>(gps.signal_strength)) + "%)";
                terminalRenderer_.addAlert(AlertLevel::Warning, msg);
            }
        }
        // This assignment happens unconditionally as long as the sensor isn't stale,
        // so you don't need a separate 'else if' branch for it.
        last_alerted_gps_signal_ = gps.signal_strength;
    }
}

IngestionEngine::~IngestionEngine() {
    stop(); // Safely signs off and joins both threads before anything else deletes
}
