#pragma once
#include <iomanip>
#include <sstream>
#include "Panel.h"
#include "src/display/TerminalLayout.h"
#include "src/sensors/FlightController.h"
#include "src/sensors/SensorStates.h"

class FlightControllerPanel : public Panel {
public:
    explicit FlightControllerPanel(const FlightController& fc) :
        fc_(fc) {
    }

    void render(std::ostream& out) override {
        drawSectionTitle(out, "🛩  FLIGHT CONTROLLER");

        const FlightControllerState state = fc_.state();
        if (state.stale) {
            out << "    Waiting for data..." << TerminalControl::ERASE_EOL << "\n";
            return;
        }

        std::ostringstream alt, vertSpeed, horizSpeed;
        alt << std::fixed << std::setprecision(1) << state.altitude_m << " m";
        vertSpeed << std::fixed << std::setprecision(1) << state.vertical_speed_mps << " m/s";
        horizSpeed << std::fixed << std::setprecision(1) << state.horizontal_speed_mps << " m/s";

        const bool offline = state.isOffline(RenderConfig::OFFLINE_TIMEOUT_SEC);
        const AnsiColor statusColor = offline ? AnsiColor::Red : AnsiColor::Green;
        const std::string statusText = offline ? "OFFLINE" : "NORMAL";

        writeRow(out, "Altitude", alt.str());
        writeRow(out, "Vertical Speed", vertSpeed.str());
        writeRow(out, "Horizontal Speed", horizSpeed.str());
        writeRow(out, "Status", statusText, statusColor, AnsiStyle::Bold);
    }

private:
    const FlightController& fc_; // Reference to state held inside IngestionEngine snapshot
};
