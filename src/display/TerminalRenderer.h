// Ensures this header file is only included once per compilation unit to prevent redefinition errors
#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "./panels/Panel.h"
#include "src/sensors/FlightController.h"

// Group related telemetry states together
struct TelemetryState {
    const FlightControllerState& fc;
    const GpsState& gps;
    const ImuState& imu;
    const BatteryState& batteryState;
};

enum class AlertLevel {
    Warning,
    Critical,
};
inline const char* toString(const AlertLevel level) {
    switch (level) {
        case AlertLevel::Warning:
            return "WARNING";
        case AlertLevel::Critical:
            return "CRITICAL";
    }
    return "UNKNOWN";
}
inline std::ostream& operator<<(std::ostream& os, const AlertLevel level) {
    return os << toString(level);
}

struct Alert {
    std::string time_str;
    AlertLevel level;
    std::string message;
};

// ---------------------------------------------------------------------------
// TerminalRenderer
//
// Renders the full terminal dashboard by overwriting content in-place using
// ANSI cursor positioning (no ncurses library dependency, no screen flicker).
//
// Each render cycle builds the entire frame into an ostringstream buffer,
// then writes it to stdout in one shot — prevents partial-frame tears.
//
// Called from the render thread in IngestionEngine every ~500ms.
// All inputs are plain state snapshots — no locks held during rendering.
// ---------------------------------------------------------------------------
class TerminalRenderer {
public:
    TerminalRenderer();

    // Allow the engine to register panels at startup
    void addPanel(std::unique_ptr<Panel> panel) {
        panels_.push_back(std::move(panel));
    }

    /**
     * Main render call — takes an aggregated telemetry snapshot and network stats to draw
     * the HUD.
     *
     * @param packets_received Total telemetry packets received since startup.
     * @param packets_dropped  Total telemetry packets dropped due to corruption or network loss.
     * @param start_time       Timestamp from when the system booted or recording started.
     */
    void render(uint64_t packets_received, uint64_t packets_dropped, std::chrono::steady_clock::time_point start_time);

    /**
     * Pushes a new alert (called from IngestionEngine when thresholds are crossed).
     *
     * Note: AlertLevel is passed by value (1 byte) instead of reference. On 64-bit architectures,
     * references use 8-byte pointers under the hood, which would add unnecessary indirection overhead here.
     *
     * @param level   The severity rating of the notification (Warning or Critical).
     * @param message The text description details of the threshold event.
     */
    void addAlert(AlertLevel level, const std::string& message);

private:
    // Panel drawing helpers — all write into out_
    void
    drawHeader(uint64_t packets_received, uint64_t packets_dropped, std::chrono::steady_clock::time_point start_time);
    void drawAlerts();
    void drawFooter();

    // Formatting helpers
    static std::string uptimeStr(std::chrono::steady_clock::time_point start_time);
    static std::string nowTimeStr();

    // Per-frame output buffer — reset at the start of render(), flushed at the end
    std::ostringstream out_;

    // Alert ring buffer — keep last 5
    // addAlert() may be called from any thread; protect with a mutex.
    static constexpr std::size_t MAX_ALERTS = 5;
    mutable std::mutex alerts_mutex_;
    std::vector<Alert> alerts_;
    std::vector<std::unique_ptr<Panel>> panels_; // Persistent panels vector
};
