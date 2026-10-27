// ---------------------------------------------------------------------------
// test_battery.cpp
//
// Tests for Battery::statusFor() threshold logic and Battery::update() /
// Battery::snapshot() round-trip.
// ---------------------------------------------------------------------------

#include <gtest/gtest.h>
#include "src/sensors/Battery.h"
#include "src/packets/WirePackets.h"

// ---------------------------------------------------------------------------
// Battery::statusFor — boundary conditions
// ---------------------------------------------------------------------------

TEST(BatteryStatus, NormalAbove30) {
    EXPECT_EQ(Battery::statusFor(100.0f), BatteryStatus::Normal);
    EXPECT_EQ(Battery::statusFor(50.0f), BatteryStatus::Normal);
    EXPECT_EQ(Battery::statusFor(30.0f), BatteryStatus::Normal);
}

TEST(BatteryStatus, WarningBetween10And29) {
    EXPECT_EQ(Battery::statusFor(29.9f), BatteryStatus::Warning);
    EXPECT_EQ(Battery::statusFor(20.0f), BatteryStatus::Warning);
    EXPECT_EQ(Battery::statusFor(10.0f), BatteryStatus::Warning);
}

TEST(BatteryStatus, CriticalBelow10) {
    EXPECT_EQ(Battery::statusFor(9.9f), BatteryStatus::Critical);
    EXPECT_EQ(Battery::statusFor(5.0f), BatteryStatus::Critical);
    EXPECT_EQ(Battery::statusFor(0.0f), BatteryStatus::Critical);
}

TEST(BatteryStatus, ExactBoundary30IsNormal) {
    // 30.0 is the lower bound of Normal — must not be Warning
    EXPECT_EQ(Battery::statusFor(30.0f), BatteryStatus::Normal);
}

TEST(BatteryStatus, JustBelow30IsWarning) {
    EXPECT_EQ(Battery::statusFor(29.99f), BatteryStatus::Warning);
}

TEST(BatteryStatus, ExactBoundary10IsWarning) {
    // 10.0 is the lower bound of Warning — must not be Critical
    EXPECT_EQ(Battery::statusFor(10.0f), BatteryStatus::Warning);
}

TEST(BatteryStatus, JustBelow10IsCritical) {
    EXPECT_EQ(Battery::statusFor(9.99f), BatteryStatus::Critical);
}

// ---------------------------------------------------------------------------
// Battery::update / snapshot — field round-trip
// ---------------------------------------------------------------------------

TEST(BatteryUpdate, SnapshotIsStaleBeforeFirstUpdate) {
    const Battery b;
    const BatteryState s = b.state();
    EXPECT_TRUE(s.stale);
}

TEST(BatteryUpdate, FieldsMatchPacketAfterUpdate) {
    Battery b;

    constexpr BatteryPacket pkt{
        .sensor_type         = SensorType::Battery,
        .timestamp_ms        = 123456789ULL,
        .charge_percentage   = 42.5f,
        .voltage             = 14.8f,
        .temperature_celsius = 35.0f
    };


    b.update(pkt);
    const BatteryState s = b.state();

    EXPECT_FALSE(s.stale);
    EXPECT_FLOAT_EQ(s.charge_percentage, 42.5f);
    EXPECT_FLOAT_EQ(s.voltage, 14.8f);
    EXPECT_FLOAT_EQ(s.temperature_celsius, 35.0f);
    EXPECT_EQ(s.timestamp_ms, 123456789ULL);
}

TEST(BatteryUpdate, SecondUpdateOverwritesFirst) {
    Battery b;

    constexpr BatteryPacket pkt1{
        .sensor_type         = SensorType::Battery,
        .timestamp_ms        = 1000ULL,
        .charge_percentage   = 80.0f,
        .voltage             = 16.0f,
        .temperature_celsius = 25.0f
    };
    b.update(pkt1);


    constexpr BatteryPacket pkt2{
        .sensor_type         = SensorType::Battery,
        .timestamp_ms        = 2000ULL,
        .charge_percentage   = 25.0f,
        .voltage             = 13.5f,
        .temperature_celsius = 40.0f
    };
    b.update(pkt2);

    const BatteryState s = b.state();
    EXPECT_FLOAT_EQ(s.charge_percentage, 25.0f);
    EXPECT_FLOAT_EQ(s.voltage, 13.5f);
    EXPECT_FLOAT_EQ(s.temperature_celsius, 40.0f);
    EXPECT_EQ(s.timestamp_ms, 2000ULL);
}

TEST(BatteryUpdate, LastUpdateTimeIsSetAfterUpdate) {
    Battery b;
    const auto before = std::chrono::steady_clock::now();
    constexpr BatteryPacket pkt{};
    b.update(pkt);
    const auto after = std::chrono::steady_clock::now();

    const BatteryState s = b.state();
    EXPECT_GE(s.last_update, before);
    EXPECT_LE(s.last_update, after);
}
