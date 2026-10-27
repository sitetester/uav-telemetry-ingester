#pragma once

#include <cstddef>
#include <cstdint>

// Identifies which sensor sent a packet — first byte of every packet.
// `0x` prefix is the standard convention for network programming and protocol definitions.
// It reminds people: "This number goes directly onto the internet wire as a raw byte."
enum class SensorType : uint8_t {
    FlightController = 0x01,
    Gps = 0x02,
    Imu = 0x03,
    Battery = 0x04,
};

// ---------------------------------------------------------------------------
// Binary Packed Packet Structs Transmitted Over UDP (Wire Format)
// ---------------------------------------------------------------------------

// #pragma pack(push, 1) activates tight, unpadded 1-byte memory alignment for the four structures
// that map directly to the raw incoming network stream (FlightControllerPacket, GpsPacket,
// ImuPacket, and BatteryPacket). No other part of system should be packed this way.
#pragma pack(push, 1)
struct FlightControllerPacket {
    SensorType sensor_type; // always SensorType::FlightController
    uint64_t timestamp_ms; // milliseconds since epoch
    float altitude_m; // metres above ground level
    float vertical_speed_mps; // m/s (mps), positive = climbing
    float horizontal_speed_mps; // m/s (mps) ground velocity
};

struct GpsPacket {
    SensorType sensor_type; // always SensorType::Gps
    uint64_t timestamp_ms; // milliseconds since epoch
    double latitude; // degrees, positive = North
    double longitude; // degrees, positive = East
    float signal_strength; // satellite reception quality percentage 0–100
};

struct ImuPacket {
    SensorType sensor_type; // always SensorType::Imu
    uint64_t timestamp_ms; // milliseconds since epoch
    float roll_deg; // rotation around front-back axis (left/right tilt)
    float pitch_deg; // rotation around side-side axis (forward/back tilt)
    float yaw_deg; // rotation around vertical axis (0-360 compass heading)
};

struct BatteryPacket {
    SensorType sensor_type; // always SensorType::Battery
    uint64_t timestamp_ms; // milliseconds since epoch
    float charge_percentage; // capacity percentage 0–100
    float voltage; // volts direct current
    float temperature_celsius; // Celsius core thermal reading
};
#pragma pack(pop) // The 1-byte constraint stops here! effectively restoring compiler back
// to its optimal default settings for all subsequent code.

// Compilation layout validations confirming protocol structures stay immutable
static_assert(sizeof(FlightControllerPacket) == 21,
    "FlightControllerPacket size mismatch!");
static_assert(sizeof(GpsPacket) == 29,
    "GpsPacket size mismatch!");
static_assert(sizeof(ImuPacket) == 21,
    "ImuPacket size mismatch!");
static_assert(sizeof(BatteryPacket) == 21,
    "BatteryPacket size mismatch!");

// Maximum UDP payload size used to instantiate localized thread buffer limits
static constexpr std::size_t MAX_PACKET_SIZE = sizeof(GpsPacket);
