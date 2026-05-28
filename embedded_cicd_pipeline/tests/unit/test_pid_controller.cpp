#include <gtest/gtest.h>
#include "pid/PIDController.h"

class PIDControllerTest : public ::testing::Test {
protected:
    PIDConfig defaultConfig{
        0.5f,  // Kp
        0.1f,  // Ki
        0.05f, // Kd
        0.01f, // Ts
        0.0f,  // u_min
        100.0f,// u_max
        50.0f  // integral_max
    };
    PIDController pid{defaultConfig};
};

TEST_F(PIDControllerTest, ZeroError_OutputIsZero) {
    float u = pid.compute(10.0f, 10.0f);
    EXPECT_NEAR(u, 0.0f, 1e-5f);
}

TEST_F(PIDControllerTest, ProportionalOnly_MatchesKpE) {
    defaultConfig.Ki = 0.0f;
    defaultConfig.Kd = 0.0f;
    PIDController p_only(defaultConfig);
    float u = p_only.compute(20.0f, 10.0f); // e = 10
    EXPECT_NEAR(u, 5.0f, 1e-5f); // Kp * e = 0.5 * 10 = 5.0
}

TEST_F(PIDControllerTest, IntegralAccumulates_OverTime) {
    defaultConfig.Kp = 0.0f;
    defaultConfig.Kd = 0.0f;
    defaultConfig.Ki = 1.0f;
    PIDController i_only(defaultConfig);
    
    // e = 1, Ts = 0.01 -> integral adds 0.01 per tick
    for (int i = 0; i < 5; ++i) {
        i_only.compute(1.0f, 0.0f);
    }
    
    EXPECT_NEAR(i_only.integralState(), 0.05f, 1e-5f);
    EXPECT_NEAR(i_only.compute(1.0f, 0.0f), 0.06f, 1e-5f);
}

TEST_F(PIDControllerTest, AntiWindup_ClampsIntegral) {
    defaultConfig.Kp = 0.0f;
    defaultConfig.Kd = 0.0f;
    defaultConfig.Ki = 1.0f;
    defaultConfig.integral_max = 10.0f;
    PIDController i_only(defaultConfig);
    
    for (int i = 0; i < 2000; ++i) {
        i_only.compute(10.0f, 0.0f); // e = 10 -> adds 0.1 per tick
    }
    
    EXPECT_NEAR(i_only.integralState(), 10.0f, 1e-5f);
}

TEST_F(PIDControllerTest, DerivativeKick_OnStepChange) {
    defaultConfig.Kp = 0.0f;
    defaultConfig.Ki = 0.0f;
    defaultConfig.Kd = 0.1f;
    PIDController d_only(defaultConfig);
    
    float u = d_only.compute(100.0f, 0.0f); // e jumps from 0 to 100
    // derivative = (100 - 0) / 0.01 = 10000
    // Kd * 10000 = 0.1 * 10000 = 1000
    EXPECT_NEAR(u, 100.0f, 1e-5f); // Clamped by u_max = 100
    
    defaultConfig.u_max = 2000.0f;
    PIDController d_only_no_clamp(defaultConfig);
    float u2 = d_only_no_clamp.compute(100.0f, 0.0f);
    EXPECT_NEAR(u2, 1000.0f, 1e-5f);
}

TEST_F(PIDControllerTest, Reset_ClearsAccumulators) {
    pid.compute(10.0f, 0.0f);
    EXPECT_NE(pid.integralState(), 0.0f);
    EXPECT_NE(pid.lastError(), 0.0f);
    
    pid.reset();
    EXPECT_NEAR(pid.integralState(), 0.0f, 1e-5f);
    EXPECT_NEAR(pid.lastError(), 0.0f, 1e-5f);
}

TEST_F(PIDControllerTest, OutputClamp_UpperBound) {
    defaultConfig.u_max = 1.0f;
    PIDController clamp_pid(defaultConfig);
    float u = clamp_pid.compute(1000.0f, 0.0f);
    EXPECT_NEAR(u, 1.0f, 1e-5f);
}

TEST_F(PIDControllerTest, OutputClamp_LowerBound) {
    defaultConfig.u_min = 0.0f;
    PIDController clamp_pid(defaultConfig);
    float u = clamp_pid.compute(-1000.0f, 0.0f);
    EXPECT_NEAR(u, 0.0f, 1e-5f);
}

TEST_F(PIDControllerTest, StepResponse_Converges) {
    float setpoint = 3000.0f;
    float rpm = 0.0f;
    
    for (int i = 0; i < 500; ++i) {
        float u = pid.compute(setpoint, rpm);
        rpm += u; // simple integrator plant for test
    }
    
    EXPECT_NEAR(rpm, setpoint, setpoint * 0.05f); // Within 5%
}

TEST_F(PIDControllerTest, SteadyState_ZeroSteadyStateError) {
    defaultConfig.u_max = 200.0f; // Increase to allow u = 110
    defaultConfig.integral_max = 5000.0f; // Increase to allow Ki*I = 110
    defaultConfig.Kd = 0.0f; // Disable derivative to prevent oscillations with instant plant
    PIDController pid2(defaultConfig);
    float setpoint = 100.0f;
    float disturbance = 10.0f;
    float rpm = 0.0f;
    
    for (int i = 0; i < 10000; ++i) {
        float u = pid2.compute(setpoint, rpm);
        rpm = u - disturbance; // P+I should eventually overcome disturbance
    }
    
    EXPECT_NEAR(rpm, setpoint, 1.0f);
}

TEST_F(PIDControllerTest, KpOnly_LinearResponse) {
    defaultConfig.Ki = 0.0f;
    defaultConfig.Kd = 0.0f;
    PIDController p_only(defaultConfig);
    
    float u1 = p_only.compute(10.0f, 0.0f);
    float u2 = p_only.compute(20.0f, 0.0f);
    
    EXPECT_NEAR(u2, 2.0f * u1, 1e-5f);
}

TEST_F(PIDControllerTest, Ts_ScalesDerivativeCorrectly) {
    defaultConfig.Kp = 0.0f;
    defaultConfig.Ki = 0.0f;
    defaultConfig.Kd = 1.0f;
    defaultConfig.Ts = 0.01f;
    defaultConfig.u_max = 10000.0f; // Prevent clamping
    PIDController d1(defaultConfig);
    
    defaultConfig.Ts = 0.02f;
    PIDController d2(defaultConfig);
    
    float u1 = d1.compute(10.0f, 0.0f);
    float u2 = d2.compute(10.0f, 0.0f);
    
    EXPECT_NEAR(u1, 2.0f * u2, 1e-5f);
}
