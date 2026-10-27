// ---------------------------------------------------------------------------
// test_packet_layout.cpp
//
// Verifies that every packet struct has the correct wire size and that
// SensorId enum values match the documented byte values.
//
// These tests guard against accidental field-type changes that would silently
// break the binary protocol between simulator and ingester.
// ---------------------------------------------------------------------------

#include <gtest/gtest.h>
#include "src/packets/WirePackets.h"

// ---------------------------------------------------------------------------
// Wire sizes
// ---------------------------------------------------------------------------

TEST(PacketLayout, FlightControllerPacketSize) {
    // 1 (SensorId) + 8 (timestamp) + 4 (altitude) + 4 (vert_spd) + 4 (horiz_spd) = 21
    EXPECT_EQ(sizeof(FlightControllerPacket), 21u);
}

TEST(PacketLayout, GpsPacketSize) {
    // 1 (SensorId) + 8 (timestamp) + 8 (latitude) + 8 (longitude) + 4 (signal) = 29
    EXPECT_EQ(sizeof(GpsPacket), 29u);
}

TEST(PacketLayout, ImuPacketSize) {
    // 1 (SensorId) + 8 (timestamp) + 4 (roll) + 4 (pitch) + 4 (yaw) = 21
    EXPECT_EQ(sizeof(ImuPacket), 21u);
}

TEST(PacketLayout, BatteryPacketSize) {
    // 1 (SensorId) + 8 (timestamp) + 4 (charge) + 4 (voltage) + 4 (temp) = 21
    EXPECT_EQ(sizeof(BatteryPacket), 21u);
}

TEST(PacketLayout, MaxPacketSizeIsGps) {
    // GpsPacket is the largest due to double fields — MAX_PACKET_SIZE must reflect this
    EXPECT_EQ(MAX_PACKET_SIZE, sizeof(GpsPacket));
    EXPECT_GE(MAX_PACKET_SIZE, sizeof(FlightControllerPacket));
    EXPECT_GE(MAX_PACKET_SIZE, sizeof(ImuPacket));
    EXPECT_GE(MAX_PACKET_SIZE, sizeof(BatteryPacket));
}

// ---------------------------------------------------------------------------
// SensorId discriminant values (must match the documented wire protocol)
// ---------------------------------------------------------------------------

TEST(PacketLayout, SensorIdValues) {
    EXPECT_EQ(static_cast<uint8_t>(SensorType::FlightController), 0x01u);
    EXPECT_EQ(static_cast<uint8_t>(SensorType::Gps), 0x02u);
    EXPECT_EQ(static_cast<uint8_t>(SensorType::Imu), 0x03u);
    EXPECT_EQ(static_cast<uint8_t>(SensorType::Battery), 0x04u);
}

// ---------------------------------------------------------------------------
// Field offsets — ensure #pragma pack(1) is actually removing padding
// ---------------------------------------------------------------------------

TEST(PacketLayout, FlightControllerFieldOffsets) {
    EXPECT_EQ(offsetof(FlightControllerPacket, sensor_type), 0u);
    EXPECT_EQ(offsetof(FlightControllerPacket, timestamp_ms), 1u);
    EXPECT_EQ(offsetof(FlightControllerPacket, altitude_m), 9u);
    EXPECT_EQ(offsetof(FlightControllerPacket, vertical_speed_mps), 13u);
    EXPECT_EQ(offsetof(FlightControllerPacket, horizontal_speed_mps), 17u);
}

TEST(PacketLayout, GpsFieldOffsets) {
    EXPECT_EQ(offsetof(GpsPacket, sensor_type), 0u);
    EXPECT_EQ(offsetof(GpsPacket, timestamp_ms), 1u);
    EXPECT_EQ(offsetof(GpsPacket, latitude), 9u);
    EXPECT_EQ(offsetof(GpsPacket, longitude), 17u);
    EXPECT_EQ(offsetof(GpsPacket, signal_strength), 25u);
}

TEST(PacketLayout, ImuFieldOffsets) {
    EXPECT_EQ(offsetof(ImuPacket, sensor_type), 0u);
    EXPECT_EQ(offsetof(ImuPacket, timestamp_ms), 1u);
    EXPECT_EQ(offsetof(ImuPacket, roll_deg), 9u);
    EXPECT_EQ(offsetof(ImuPacket, pitch_deg), 13u);
    EXPECT_EQ(offsetof(ImuPacket, yaw_deg), 17u);
}

TEST(PacketLayout, BatteryFieldOffsets) {
    EXPECT_EQ(offsetof(BatteryPacket, sensor_type), 0u);
    EXPECT_EQ(offsetof(BatteryPacket, timestamp_ms), 1u);
    EXPECT_EQ(offsetof(BatteryPacket, charge_percentage), 9u);
    EXPECT_EQ(offsetof(BatteryPacket, voltage), 13u);
    EXPECT_EQ(offsetof(BatteryPacket, temperature_celsius), 17u);
}
