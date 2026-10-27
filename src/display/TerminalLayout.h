#pragma once

#include <cstdint>
#include <string_view>

enum class AnsiStyle : uint8_t {
    None = 0,
    Bold = 1,
    Dim = 2,
};

constexpr std::string_view toEscape(const AnsiStyle style) {
    switch (style) {
        case AnsiStyle::Bold:
            return "\033[1m";
        case AnsiStyle::Dim:
            return "\033[2m";
        default:
            return "";
    }
}

// Auto-convert AnsiStyle when pushed into any std::ostream
inline std::ostream& operator<<(std::ostream& os, const AnsiStyle style) {
    return os << toEscape(style);
}

enum class AnsiColor : uint8_t {
    Default = 39,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Cyan = 36,
    White = 97,
};

constexpr std::string_view toEscape(const AnsiColor color) {
    switch (color) {
        case AnsiColor::Red:
            return "\033[31m";
        case AnsiColor::Green:
            return "\033[32m";
        case AnsiColor::Yellow:
            return "\033[33m";
        case AnsiColor::Cyan:
            return "\033[36m";
        case AnsiColor::White:
            return "\033[97m";
        default:
            return "\033[39m"; // Handles Default and invalid typecasts
    }
}
// Auto-convert AnsiColor when pushed into any std::ostream
inline std::ostream& operator<<(std::ostream& os, const AnsiColor color) {
    return os << toEscape(color);
}

namespace TerminalControl {
    // Resets all text attributes (colors, bold, dim) back to the terminal default state.
    constexpr auto RESET = "\033[0m";
    // Teleports the terminal cursor to the absolute top-left corner (0,0) without clearing text.
    constexpr auto CURSOR_HOME = "\033[H";
    // Makes the terminal cursor invisible to prevent flickering during redraws.
    constexpr auto HIDE_CURSOR = "\033[?25l";
    // Restores the visibility of the blinking terminal cursor.
    constexpr auto SHOW_CURSOR = "\033[?25h";
    // Erases all text from the current cursor position to the very end of the active line.
    constexpr auto ERASE_EOL = "\033[K";
    // Wipes the entire visible terminal screen canvas clean.
    constexpr auto CLEAR_SCREEN = "\033[2J";
}

struct RenderConfig {
    static constexpr int HEADER_LENGTH = 85;
    static constexpr double OFFLINE_TIMEOUT_SEC = 3.0;
};

inline void drawSectionTitle( //
    std::ostream& out,
    const std::string_view title) {
    out << "\n"
        << AnsiStyle::Bold << AnsiColor::White << "  " << title << TerminalControl::RESET << TerminalControl::ERASE_EOL
        << "\n";
}

// Count visible characters in a UTF-8 string (skips continuation bytes).
inline int visibleLen(const std::string_view& s) {
    int count = 0;
    for (const unsigned char c: s) {
        // Adding the `u` guarantees both sides of the != operator are unsigned
        if ((c & 0xC0u) != 0x80u)
            ++count;
    }
    return count;
}

inline std::string padTo( //
    const std::string_view& text,
    const int width) {
    const int len = visibleLen(text);
    if (len >= width)
        return std::string(text);
    return std::string(text) + std::string(width - len, ' ');
}

inline void writeRow( //
    std::ostream& out,
    const std::string_view& key,
    const std::string_view& value,
    const AnsiColor color = AnsiColor::Default,
    const AnsiStyle style = AnsiStyle::None) {
    static constexpr int KEY_WIDTH = 20;
    out << "    " << key;
    // Calculate the missing spaces and dump them straight into the stream buffer natively
    const int len = visibleLen(key);
    if (len < KEY_WIDTH) {
        out << std::string_view("                    ", KEY_WIDTH - len);
    }

    if (style != AnsiStyle::None)
        out << style;

    out << color << value << TerminalControl::RESET << TerminalControl::ERASE_EOL << "\n";
}
