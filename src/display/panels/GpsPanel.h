#pragma once
#include <iomanip>
#include <sstream>
#include "src/display/TerminalLayout.h"
#include "Panel.h"

class GpsPanel : public Panel {
public:
    explicit GpsPanel(const Gps& gps) :
        gps_(gps) {
    }

    void render(std::ostream& out) override {
        drawSectionTitle(out, "📡 GPS");

        const GpsState state = gps_.state();
        if (state.stale) {
            out << "    Waiting for data..." << TerminalControl::ERASE_EOL << "\n";
            return;
        }

        std::ostringstream lat, lon, sig;
        lat << std::fixed << std::setprecision(4) << std::abs(state.latitude) << "°  "
            << (state.latitude >= 0 ? "N" : "S");
        lon << std::fixed << std::setprecision(4) << std::abs(state.longitude) << "°  "
            << (state.longitude >= 0 ? "E" : "W");
        sig << std::fixed << std::setprecision(0) << state.signal_strength << "%";

        const AnsiColor sigColor = (state.signal_strength < 40.0f) ? AnsiColor::Yellow : AnsiColor::Green;

        AnsiColor statusColor;
        std::string statusText;

        if (state.isOffline(RenderConfig::OFFLINE_TIMEOUT_SEC)) {
            statusColor = AnsiColor::Red;
            statusText = "OFFLINE";
        } else if (state.signal_strength < 30.0f) {
            statusColor = AnsiColor::Red;
            statusText = "WEAK SIGNAL";
        } else if (state.signal_strength < 60.0f) {
            statusColor = AnsiColor::Yellow;
            statusText = "DEGRADED";
        } else {
            statusColor = AnsiColor::Green;
            statusText = "NORMAL";
        }

        writeRow(out, "Latitude", lat.str());
        writeRow(out, "Longitude", lon.str());
        writeRow(out, "Signal Strength", sig.str(), sigColor);
        writeRow(out, "Status", statusText, statusColor, AnsiStyle::Bold);
    }


private:
    const Gps& gps_; // Reference to state held inside IngestionEngine snapshot
};
