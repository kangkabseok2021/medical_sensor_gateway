#include <gtest/gtest.h>
#include "app/ProcessingLoop.h"

class MockMotorHAL : public IMotorHAL {
public:
    float rpm_to_return{0.0f};
    bool fault_state{false};
    float last_pwm_written{-1.0f};

    float readRPM() const override { return rpm_to_return; }
    void writePWM(float duty) override { last_pwm_written = duty; }
    bool isFault() const override { return fault_state; }
    void reset() override { last_pwm_written = -1.0f; }
};

class ProcessingLoopTest : public ::testing::Test {

    PIDConfig config{0.1f, 0.01f, 0.001f, 0.001f, 0.0f, 1.0f, 100.0f};
    PIDController pid{config};
};

TEST_F(ProcessingLoopTest, Tick_WritesPWM_WhenNoFault) {
    ProcessingLoop loop(hal, pid, 2000.0f);
    hal.rpm_to_return = 1000.0f;
    
    loop.tick();
    
    EXPECT_GT(hal.last_pwm_written, 0.0f);
}

TEST_F(ProcessingLoopTest, Tick_WriteZeroPWM_OnFault) {
    ProcessingLoop loop(hal, pid, 2000.0f);
    hal.fault_state = true;
    hal.rpm_to_return = 1000.0f;
    
    loop.tick();
    
    EXPECT_FLOAT_EQ(hal.last_pwm_written, 0.0f);
}

TEST_F(ProcessingLoopTest, SetSetpoint_AffectsOutput) {
    ProcessingLoop loop(hal, pid, 2000.0f);
    hal.rpm_to_return = 1000.0f;
    loop.tick();
    float output1 = loop.lastOutput();
    
    loop.setSetpoint(0.0f);
    loop.tick();
    float output2 = loop.lastOutput();
    
    EXPECT_NE(output1, output2);
}

TEST_F(ProcessingLoopTest, TickCount_IncrementsByOne_PerTick) {
    ProcessingLoop loop(hal, pid, 2000.0f);
    EXPECT_EQ(loop.tickCount(), 0);
    
    for (int i = 0; i < 100; ++i) {
        loop.tick();
    }
    
    EXPECT_EQ(loop.tickCount(), 100);
}

TEST_F(ProcessingLoopTest, FaultRecovery_ResumesOutput) {
    ProcessingLoop loop(hal, pid, 2000.0f);
    hal.rpm_to_return = 1000.0f;
    
    hal.fault_state = true;
    for (int i = 0; i < 10; ++i) loop.tick();
    EXPECT_FLOAT_EQ(loop.lastOutput(), 0.0f);
    
    hal.fault_state = false;
    loop.tick();
    EXPECT_GT(loop.lastOutput(), 0.0f);
}

TEST_F(ProcessingLoopTest, ConvergenceTest_50Ticks) {
    ProcessingLoop loop(hal, pid, 3000.0f);
    hal.rpm_to_return = 0.0f;
    
    // First tick error
    loop.tick();
    float error0 = 3000.0f - hal.rpm_to_return;
    
    // Simulate plant
    for (int i = 0; i < 50; ++i) {
        hal.rpm_to_return += hal.last_pwm_written * 100.0f;
        loop.tick();
    }
    
    float error50 = 3000.0f - hal.rpm_to_return;
    EXPECT_LT(std::abs(error50), std::abs(error0));
}
