// ---------------------------------------------------------------------------
// test_sensors.cpp
//
// Tests for FlightController, Gps, and Imu — update() / snapshot() round-trip.
// All three follow the same mutex-guarded producer/consumer pattern, so the
// tests are structurally identical but verify each sensor's own fields.
// ---------------------------------------------------------------------------

#include <gtest/gtest.h>
#include "src/sensors/FlightController.h"
#include "src/sensors/Gps.h"
#include "src/sensors/Imu.h"
#include "src/packets/WirePackets.h"

// ===========================================================================
// FlightController
// ===========================================================================

TEST(FlightControllerSensor, SnapshotIsStaleBeforeFirstUpdate) {
    const FlightController fc;
    EXPECT_TRUE(fc.state().stale);
}

TEST(FlightControllerSensor, FieldsMatchPacketAfterUpdate) {
    FlightController fc;

    constexpr FlightControllerPacket pkt{
        .sensor_type          = SensorType::FlightController,
        .timestamp_ms         = 500ULL,
        .altitude_m           = 75.3f,
        .vertical_speed_mps   = 1.2f,
        .horizontal_speed_mps = 9.5f
    };


    fc.update(pkt);
    const FlightControllerState s = fc.state();

    EXPECT_FALSE(s.stale);
    EXPECT_FLOAT_EQ(s.altitude_m, 75.3f);
    EXPECT_FLOAT_EQ(s.vertical_speed_mps, 1.2f);
    EXPECT_FLOAT_EQ(s.horizontal_speed_mps, 9.5f);
    EXPECT_EQ(s.timestamp_ms, 500ULL);
}

TEST(FlightControllerSensor, SecondUpdateOverwritesFirst) {
    FlightController fc;

    constexpr FlightControllerPacket pkt1{
        .sensor_type          = SensorType::FlightController,
        .timestamp_ms         = 100ULL,
        .altitude_m           = 50.0f,
        .vertical_speed_mps   = 0.5f,
        .horizontal_speed_mps = 8.0f
    };
    fc.update(pkt1);

    constexpr FlightControllerPacket pkt2{
        .sensor_type          = SensorType::FlightController,
        .timestamp_ms         = 200ULL,
        .altitude_m           = 120.0f,
        .vertical_speed_mps   = -0.3f,
        .horizontal_speed_mps = 5.5f
    };
    fc.update(pkt2);


    const FlightControllerState s = fc.state();
    EXPECT_FLOAT_EQ(s.altitude_m, 120.0f);
    EXPECT_FLOAT_EQ(s.vertical_speed_mps, -0.3f);
    EXPECT_FLOAT_EQ(s.horizontal_speed_mps, 5.5f);
    EXPECT_EQ(s.timestamp_ms, 200ULL);
}

TEST(FlightControllerSensor, LastUpdateTimeIsSetAfterUpdate) {
    FlightController fc;
    const auto before = std::chrono::steady_clock::now();
    constexpr FlightControllerPacket pkt{};
    fc.update(pkt);
    const auto after = std::chrono::steady_clock::now();

    const FlightControllerState s = fc.state();
    EXPECT_GE(s.last_update, before);
    EXPECT_LE(s.last_update, after);
}

// ===========================================================================
// Gps
// ===========================================================================

TEST(GpsSensor, SnapshotIsStaleBeforeFirstUpdate) {
    const Gps gps;
    EXPECT_TRUE(gps.state().stale);
}

TEST(GpsSensor, FieldsMatchPacketAfterUpdate) {
    Gps gps;

    constexpr GpsPacket pkt{
        .sensor_type     = SensorType::Gps,
        .timestamp_ms    = 999ULL,
        .latitude        = 54.6872,
        .longitude       = 25.2797,
        .signal_strength = 87.5f
    };


    gps.update(pkt);
    const GpsState s = gps.state();

    EXPECT_FALSE(s.stale);
    EXPECT_DOUBLE_EQ(s.latitude, 54.6872);
    EXPECT_DOUBLE_EQ(s.longitude, 25.2797);
    EXPECT_FLOAT_EQ(s.signal_strength, 87.5f);
    EXPECT_EQ(s.timestamp_ms, 999ULL);
}

TEST(GpsSensor, SecondUpdateOverwritesFirst) {
    Gps gps;

    constexpr GpsPacket pkt1{
        .sensor_type     = SensorType::Gps,
        .timestamp_ms    = 100ULL,
        .latitude        = 54.6872,
        .longitude       = 25.2797,
        .signal_strength = 90.0f
    };
    gps.update(pkt1);

    constexpr GpsPacket pkt2{
        .sensor_type     = SensorType::Gps,
        .timestamp_ms    = 200ULL,
        .latitude        = 54.6900,
        .longitude       = 25.2810,
        .signal_strength = 25.0f // weak signal
    };
    gps.update(pkt2);


    const GpsState s = gps.state();
    EXPECT_DOUBLE_EQ(s.latitude, 54.6900);
    EXPECT_DOUBLE_EQ(s.longitude, 25.2810);
    EXPECT_FLOAT_EQ(s.signal_strength, 25.0f);
    EXPECT_EQ(s.timestamp_ms, 200ULL);
}

TEST(GpsSensor, LastUpdateTimeIsSetAfterUpdate) {
    Gps gps;
    const auto before = std::chrono::steady_clock::now();
    constexpr GpsPacket pkt{};
    gps.update(pkt);
    const auto after = std::chrono::steady_clock::now();

    const GpsState s = gps.state();
    EXPECT_GE(s.last_update, before);
    EXPECT_LE(s.last_update, after);
}

// ===========================================================================
// Imu
// ===========================================================================

TEST(ImuSensor, SnapshotIsStaleBeforeFirstUpdate) {
    const Imu imu;
    EXPECT_TRUE(imu.state().stale);
}

TEST(ImuSensor, FieldsMatchPacketAfterUpdate) {
    Imu imu;

    constexpr ImuPacket pkt{
        .sensor_type  = SensorType::Imu,
        .timestamp_ms = 777ULL,
        .roll_deg     = 5.5f,
        .pitch_deg    = -3.2f,
        .yaw_deg      = 180.0f
    };


    imu.update(pkt);
    const ImuState s = imu.state();

    EXPECT_FALSE(s.stale);
    EXPECT_FLOAT_EQ(s.roll_deg, 5.5f);
    EXPECT_FLOAT_EQ(s.pitch_deg, -3.2f);
    EXPECT_FLOAT_EQ(s.yaw_deg, 180.0f);
    EXPECT_EQ(s.timestamp_ms, 777ULL);
}

TEST(ImuSensor, SecondUpdateOverwritesFirst) {
    Imu imu;

    constexpr ImuPacket pkt1{
        .sensor_type  = SensorType::Imu,
        .timestamp_ms = 100ULL,
        .roll_deg     = 0.0f,
        .pitch_deg    = 0.0f,
        .yaw_deg      = 90.0f
    };
    imu.update(pkt1);

    constexpr ImuPacket pkt2{
        .sensor_type  = SensorType::Imu,
        .timestamp_ms = 200ULL,
        .roll_deg     = 12.0f,
        .pitch_deg    = -8.0f,
        .yaw_deg      = 270.0f
    };
    imu.update(pkt2);


    const ImuState s = imu.state();
    EXPECT_FLOAT_EQ(s.roll_deg, 12.0f);
    EXPECT_FLOAT_EQ(s.pitch_deg, -8.0f);
    EXPECT_FLOAT_EQ(s.yaw_deg, 270.0f);
    EXPECT_EQ(s.timestamp_ms, 200ULL);
}

TEST(ImuSensor, LastUpdateTimeIsSetAfterUpdate) {
    Imu imu;
    const auto before = std::chrono::steady_clock::now();
    constexpr ImuPacket pkt{};
    imu.update(pkt);
    const auto after = std::chrono::steady_clock::now();

    const ImuState s = imu.state();
    EXPECT_GE(s.last_update, before);
    EXPECT_LE(s.last_update, after);
}
