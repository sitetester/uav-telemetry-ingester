#include "TerminalRenderer.h"

#include <iomanip>
#include <iostream>
#include <ranges>
#include <sstream>

#include "./TerminalLayout.h"
#include "./panels/Panel.h"

TerminalRenderer::TerminalRenderer() {
    alerts_.reserve(MAX_ALERTS);
}

/**
 * @brief Draws the terminal interface by processing the latest telemetry snapshot and data stream health.
 *
 * @details Buffers all layout commands—including screen clearing and cursor resets—in memory
 *          before writing everything to standard output. This eliminates intermediate blank
 *          states and results in a smooth, flicker-free terminal interface.
 *
 * @param packets_received Total telemetry packets received since startup.
 * @param packets_dropped  Total telemetry packets dropped due to corruption or network loss.
 * @param start_time       Timestamp from when the system booted or recording started.
 */
void TerminalRenderer::render( //
    const uint64_t packets_received,
    const uint64_t packets_dropped,
    const std::chrono::steady_clock::time_point start_time) {
    out_.str(std::string());
    out_.clear();
    // Full clear + home inside the buffer — sent atomically with the frame so
    // the terminal never shows a blank intermediate state.
    out_ << TerminalControl::HIDE_CURSOR << "\033[2J" << TerminalControl::CURSOR_HOME;

    // draw terminal layout
    drawHeader(packets_received, packets_dropped, start_time);
    for (const auto& panel: panels_) {
        panel->render(out_);
    }
    drawAlerts();
    drawFooter();

    out_ << TerminalControl::SHOW_CURSOR;
    std::cout << out_.str();
    std::cout.flush();
}

void TerminalRenderer::addAlert(AlertLevel level, const std::string& message) {
    Alert a;
    a.time_str = nowTimeStr();
    a.level = level;
    a.message = message;

    std::lock_guard lock(alerts_mutex_);
    if (alerts_.size() >= MAX_ALERTS) {
        alerts_.erase(alerts_.begin());
    }
    alerts_.push_back(std::move(a));
}

void TerminalRenderer::drawHeader(const uint64_t packets_received,
    const uint64_t packets_dropped,
    const std::chrono::steady_clock::time_point start_time) {
    const std::string sep(RenderConfig::HEADER_LENGTH, '=');

    out_ << AnsiStyle::Bold << AnsiColor::Cyan << "  " << sep << TerminalControl::ERASE_EOL << "\n";
    out_ << "  🛸  UAV REAL-TIME TELEMETRY INGESTION ENGINE  |  STATUS: ACTIVE" << TerminalControl::ERASE_EOL << "\n";
    out_ << "  " << sep << TerminalControl::RESET << TerminalControl::ERASE_EOL << "\n";
    out_ << "  System Time: " << nowTimeStr() << "  |  Uptime: " << uptimeStr(start_time)
         << "  |  Packets: " << packets_received;
    if (packets_dropped > 0) {
        out_ << AnsiColor::Yellow << "  |  Dropped: " << packets_dropped << TerminalControl::RESET;
    }
    out_ << TerminalControl::ERASE_EOL << "\n";
}

void TerminalRenderer::drawAlerts() {
    drawSectionTitle(out_, "⚠️   RECENT ALERTS");

    // Lock the shared resource directly over the immediate print pass
    std::lock_guard lock(alerts_mutex_);

    // Read directly from live memory bounds without allocating deep vector copies
    if (alerts_.empty()) {
        out_ << "    " << AnsiStyle::Dim << "No alerts" << TerminalControl::RESET << TerminalControl::ERASE_EOL << "\n";
        return;
    }

    // Stream data directly out of the live buffer slots
    for (const auto& [time_str, level, message]: std::views::reverse(alerts_)) {
        auto color = AnsiColor::White;
        std::string levelStr = "[INFO]    ";

        if (level == AlertLevel::Critical) {
            color = AnsiColor::Red;
            levelStr = "[CRITICAL]";
        } else if (level == AlertLevel::Warning) {
            color = AnsiColor::Yellow;
            levelStr = "[WARNING] ";
        }

        out_ << "    " << AnsiStyle::Dim << time_str << TerminalControl::RESET << "  " << color << AnsiStyle::Bold
             << levelStr << TerminalControl::RESET << "  " << message << TerminalControl::ERASE_EOL << "\n";
    }
}

void TerminalRenderer::drawFooter() {
    const std::string sep(RenderConfig::HEADER_LENGTH, '=');

    out_ << "\n"
         << AnsiStyle::Bold << AnsiColor::Cyan << "  " << sep << TerminalControl::RESET << TerminalControl::ERASE_EOL
         << "\n";

    out_ << "  " << AnsiStyle::Dim << "[Press Ctrl+C to stop]" << TerminalControl::RESET << TerminalControl::ERASE_EOL
         << "\n";
}

std::string TerminalRenderer::uptimeStr(const std::chrono::steady_clock::time_point start_time) {
    using namespace std::chrono;
    const auto elapsed = duration_cast<seconds>(steady_clock::now() - start_time).count();
    const int h = static_cast<int>(elapsed / 3600);
    const int m = static_cast<int>((elapsed % 3600) / 60);
    const int s = static_cast<int>(elapsed % 60);
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << h << ":" << std::setw(2) << m << ":" << std::setw(2) << s;
    return ss.str();
}

std::string TerminalRenderer::nowTimeStr() {
    const std::time_t now = std::time(nullptr);
    std::tm tm_buf{};

    localtime_r(&now, &tm_buf);

    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
