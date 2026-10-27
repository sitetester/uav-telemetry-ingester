#pragma once
#include <iomanip>
#include <sstream>
#include "Panel.h"
#include "src/display/TerminalLayout.h"

class ImuPanel : public Panel {
public:
    explicit ImuPanel(const Imu& imu) :
        imu_(imu) {
    }

    void render(std::ostream& out) override {
        drawSectionTitle(out, "🧭 IMU (Orientation)");

        ImuState state = imu_.state();
        if (state.stale) {
            out << "    Waiting for data..." << TerminalControl::ERASE_EOL << "\n";
            return;
        }

        std::ostringstream roll, pitch, yaw;
        roll << std::fixed << std::setprecision(1) << state.roll_deg << "°   (left/right tilt)";
        pitch << std::fixed << std::setprecision(1) << state.pitch_deg << "°   (forward/back tilt)";
        yaw << std::fixed << std::setprecision(1) << state.yaw_deg << "°   (compass heading)";

        const bool offline = state.isOffline(RenderConfig::OFFLINE_TIMEOUT_SEC);

        const AnsiColor statusColor = offline ? AnsiColor::Red : AnsiColor::Green;
        const std::string statusText = offline ? "OFFLINE" : "NORMAL";

        writeRow(out, "Roll", roll.str());
        writeRow(out, "Pitch", pitch.str());
        writeRow(out, "Yaw", yaw.str());
        writeRow(out, "Status", statusText, statusColor, AnsiStyle::Bold);
    }

private:
    const Imu& imu_; // Reference to state held inside IngestionEngine snapshot
};
