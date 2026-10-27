#pragma once
#include "Panel.h"
#include "src/sensors/Battery.h"
#include "../TerminalLayout.h"
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

class BatteryPanel : public Panel {
public:
    // Receives a reference to the active, live background sensor processing instance
    explicit BatteryPanel(const Battery& battery) : battery_sensor_(battery) {}

    void render(std::ostream& out) override {
        drawSectionTitle(out, "🔋 BATTERY");

        // Fetch a thread-safe static frame snapshot copy from the processor
        const BatteryState state = battery_sensor_.state();
        if (state.stale) {
            out << "    Waiting for data..." << TerminalControl::ERASE_EOL << "\n";
            return;
        }

        const BatteryStatus status = Battery::statusFor(state.charge_percentage);

        std::ostringstream chargeLine, volt, temp;
        chargeLine << std::fixed << std::setprecision(0) << state.charge_percentage << "%  "
                   << batteryBar(state.charge_percentage);

        volt << std::fixed << std::setprecision(2) << state.voltage << " V";
        temp << std::fixed << std::setprecision(1) << state.temperature_celsius << " °C";

        AnsiColor statusColor;
        std::string statusStr;

        if (state.isOffline(RenderConfig::OFFLINE_TIMEOUT_SEC)) {
            statusColor = AnsiColor::Red;
            statusStr = "OFFLINE";
        } else if (status == BatteryStatus::Critical) {
            statusColor = AnsiColor::Red;
            statusStr = "CRITICAL — Land immediately!";
        } else if (status == BatteryStatus::Warning) {
            statusColor = AnsiColor::Yellow;
            statusStr = "LOW — Return to base soon!";
        } else {
            statusColor = AnsiColor::Green;
            statusStr = "NORMAL";
        }

        // Determine clean inline bar color mapping based on status limits
        const AnsiColor chargeColor = (status == BatteryStatus::Critical) ? AnsiColor::Red
                                    : ((status == BatteryStatus::Warning) ? AnsiColor::Yellow : AnsiColor::Green);

        writeRow(out, "Charge", chargeLine.str(), chargeColor);
        writeRow(out, "Voltage", volt.str());
        writeRow(out, "Temperature", temp.str());
        writeRow(out, "Status", statusStr, statusColor, AnsiStyle::Bold);
    }

private:
    // Renders the local visual ASCII remaining charge representation gauge blocks
    static std::string batteryBar(const float charge_percentage, const int width = 20) {
        int filled = static_cast<int>(std::round(charge_percentage / 100.0f * static_cast<float>(width)));
        filled = std::clamp(filled, 0, width);
        std::string bar = "[";
        for (int i = 0; i < width; ++i)
            bar += (i < filled) ? "\xE2\x96\x88" : "\xE2\x96\x91"; // Handles raw UTF-8 string blocks
        bar += ']';
        return bar;
    }

    const Battery& battery_sensor_; // Holds a reference to the active hardware memory component
};
