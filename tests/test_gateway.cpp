#include "../src/ButterworthFilter.h"
#include "../src/ECGSensor.h"
#include "../src/FlowSensor.h"
#include "../src/MedicalFsm.h"
#include "../src/SensorGateway.h"
#include <gtest/gtest.h>
#include <cmath>
#include <memory>

// ── Helpers ──────────────────────────────────────────────────────────────────

// Advance FSM through INITIALISING and CALIBRATING to reach MONITORING.
static void reach_monitoring(MedicalFsm& fsm) {
    // Drive INITIALISING → CALIBRATING → MONITORING
    SensorReadings ok{1.0, 75.0};
    for (int i = 0; i < MedicalFsm::kCalibSamples + 2; ++i)
        fsm.update(ok);
    ASSERT_EQ(fsm.state(), MedState::MONITORING);
}

// ── SensorTest (3) ───────────────────────────────────────────────────────────

TEST(SensorTest, FlowSensorInDomain) {
    SimulatedFlowSensor f(1.0, 0);
    for (int i = 0; i < 1000; ++i) {
        double v = f.read();
        EXPECT_GT(v, 0.0) << "flow should be positive at nominal rate";
    }
}

TEST(SensorTest, ECGSensorHeartRateRange) {
    SimulatedECGSensor e(75.0, 0);
    for (int i = 0; i < 1000; ++i) {
        double v = e.read();
        EXPECT_GT(v, 40.0) << "HR should be above bradycardia threshold";
        EXPECT_LT(v, 150.0) << "HR should be below tachycardia threshold";
    }
}

TEST(SensorTest, SelfTestPassesAfterInit) {
    SimulatedFlowSensor f;
    SimulatedECGSensor  e;
    EXPECT_TRUE(f.selfTest());
    EXPECT_TRUE(e.selfTest());
}

// ── FilterTest (5) ───────────────────────────────────────────────────────────

TEST(FilterTest, PassbandPreserved) {
    // 1 Hz sine at 1 kHz Fs — well below 10 Hz cutoff → amplitude > 0.9
    ButterworthFilter filt(10.0, 1000.0);
    std::vector<double> in(2000);
    for (size_t i = 0; i < in.size(); ++i)
        in[i] = std::sin(2.0 * M_PI * 1.0 * i / 1000.0);
    auto out = filt.apply(in);
    double peak = 0.0;
    for (size_t i = 500; i < out.size(); ++i)
        peak = std::max(peak, std::abs(out[i]));
    EXPECT_GT(peak, 0.9);
}

TEST(FilterTest, StopbandAttenuated) {
    // 100 Hz sine — well above 10 Hz cutoff → amplitude < 0.05
    ButterworthFilter filt(10.0, 1000.0);
    std::vector<double> in(3000);
    for (size_t i = 0; i < in.size(); ++i)
        in[i] = std::sin(2.0 * M_PI * 100.0 * i / 1000.0);
    auto out = filt.apply(in);
    double peak = 0.0;
    for (size_t i = 1000; i < out.size(); ++i)
        peak = std::max(peak, std::abs(out[i]));
    EXPECT_LT(peak, 0.05);
}

TEST(FilterTest, OutputLengthMatchesInput) {
    ButterworthFilter filt(10.0, 1000.0);
    std::vector<double> in(128, 1.0);
    auto out = filt.apply(in);
    EXPECT_EQ(out.size(), in.size());
}

TEST(FilterTest, StepResponseConverges) {
    // Step input — output should eventually reach ~1.0 in passband
    ButterworthFilter filt(10.0, 1000.0);
    double last = 0.0;
    for (int i = 0; i < 1000; ++i)
        last = filt.tick(1.0);
    EXPECT_NEAR(last, 1.0, 0.01);
}

TEST(FilterTest, ZeroPaddingNoThrow) {
    ButterworthFilter filt(10.0, 1000.0);
    EXPECT_NO_THROW(filt.apply({}));
}

// ── FsmTest (10) ─────────────────────────────────────────────────────────────

TEST(FsmTest, StartsInInitialising) {
    MedicalFsm fsm;
    EXPECT_EQ(fsm.state(), MedState::INITIALISING);
}

TEST(FsmTest, CalibratingOnSelfTestPass) {
    MedicalFsm fsm;
    SensorReadings ok{1.0, 75.0};
    fsm.update(ok);  // INITIALISING → CALIBRATING
    EXPECT_EQ(fsm.state(), MedState::CALIBRATING);
}

TEST(FsmTest, SelfTestFailureGivesErrorHalt) {
    // Gateway checks selfTest() before calling update(); simulate by injecting
    // a mock sensor that fails selfTest.
    struct FailSensor : ISensor {
        double read() override { return 0.0; }
        bool selfTest() override { return false; }
        std::string_view name() const noexcept override { return "fail"; }
    };
    auto flow = std::make_shared<FailSensor>();
    auto ecg  = std::make_shared<SimulatedECGSensor>();
    auto fsm  = std::make_shared<MedicalFsm>();
    SensorGateway gw(flow, ecg, fsm, nullptr);
    bool ok = gw.self_test();
    EXPECT_FALSE(ok);
}

TEST(FsmTest, MonitoringOnCalibrationComplete) {
    MedicalFsm fsm;
    reach_monitoring(fsm);
    EXPECT_EQ(fsm.state(), MedState::MONITORING);
}

TEST(FsmTest, AlarmOnLowFlowFor5Readings) {
    MedicalFsm fsm;
    reach_monitoring(fsm);
    SensorReadings low{0.05, 75.0};  // below kFlowLowHard
    for (int i = 0; i < MedicalFsm::kFlowAlarmCount - 1; ++i) {
        fsm.update(low);
        EXPECT_EQ(fsm.state(), MedState::MONITORING);
    }
    fsm.update(low);
    EXPECT_EQ(fsm.state(), MedState::ALARM);
}

TEST(FsmTest, ErrorHaltOnOverdoseImmediate) {
    MedicalFsm fsm;
    reach_monitoring(fsm);
    SensorReadings overdose{20.0, 75.0};  // > kFlowHighHard
    fsm.update(overdose);
    EXPECT_EQ(fsm.state(), MedState::ERROR_HALT);
}

TEST(FsmTest, AlarmOnBradycardiaFor2s) {
    MedicalFsm fsm;
    reach_monitoring(fsm);
    SensorReadings brady{1.0, 20.0};  // below kHrLowHard
    for (int i = 0; i < MedicalFsm::kHrLowCount - 1; ++i)
        fsm.update(brady);
    EXPECT_EQ(fsm.state(), MedState::MONITORING);
    fsm.update(brady);
    EXPECT_EQ(fsm.state(), MedState::ALARM);
}

TEST(FsmTest, ErrorHaltOnTachycardiaFor3s) {
    MedicalFsm fsm;
    reach_monitoring(fsm);
    SensorReadings tachy{1.0, 250.0};  // > kHrHighHard
    for (int i = 0; i < MedicalFsm::kHrHighCount - 1; ++i)
        fsm.update(tachy);
    EXPECT_EQ(fsm.state(), MedState::MONITORING);
    fsm.update(tachy);
    EXPECT_EQ(fsm.state(), MedState::ERROR_HALT);
}

TEST(FsmTest, ThreeAlarmsWithoutAckGivesErrorHalt) {
    MedicalFsm fsm;
    reach_monitoring(fsm);

    // Trigger first alarm
    SensorReadings low{0.05, 75.0};
    for (int i = 0; i < MedicalFsm::kFlowAlarmCount; ++i)
        fsm.update(low);
    ASSERT_EQ(fsm.state(), MedState::ALARM);

    // Drive kMaxAlarms update cycles without ack
    SensorReadings ok{1.0, 75.0};
    for (int i = 0; i < MedicalFsm::kMaxAlarms; ++i)
        fsm.update(ok);
    EXPECT_EQ(fsm.state(), MedState::ERROR_HALT);
}

TEST(FsmTest, ResetFromErrorHaltRequiresExplicitCall) {
    MedicalFsm fsm;
    reach_monitoring(fsm);
    SensorReadings overdose{20.0, 75.0};
    fsm.update(overdose);
    ASSERT_EQ(fsm.state(), MedState::ERROR_HALT);

    // Verify update() alone does not clear ERROR_HALT
    SensorReadings ok{1.0, 75.0};
    for (int i = 0; i < 10; ++i)
        fsm.update(ok);
    EXPECT_EQ(fsm.state(), MedState::ERROR_HALT);

    // Key-switch reset restores to INITIALISING
    fsm.reset();
    EXPECT_EQ(fsm.state(), MedState::INITIALISING);
}
