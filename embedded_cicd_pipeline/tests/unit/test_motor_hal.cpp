#include <gtest/gtest.h>
#include "hal/SimulatedMotorHAL.h"

class MotorHALTest : public ::testing::Test {
protected:
    MotorRegisters registers;
    SimulatedMotorHAL hal{registers};
};

TEST_F(MotorHALTest, ReadRPM_ReturnsRegisterValue) {
    registers.encoder_rpm = 3000.0f;
    EXPECT_FLOAT_EQ(hal.readRPM(), 3000.0f);
}

TEST_F(MotorHALTest, WritePWM_StoresClampedValue) {
    hal.writePWM(0.75f);
    EXPECT_FLOAT_EQ(registers.pwm_duty, 0.75f);
}

TEST_F(MotorHALTest, WritePWM_ClampsAboveOne) {
    hal.writePWM(1.5f);
    EXPECT_FLOAT_EQ(registers.pwm_duty, 1.0f);
}

TEST_F(MotorHALTest, WritePWM_ClampsBelowZero) {
    hal.writePWM(-0.3f);
    EXPECT_FLOAT_EQ(registers.pwm_duty, 0.0f);
}

TEST_F(MotorHALTest, IsFault_ReturnsTrue_WhenFlagSet) {
    registers.fault = true;
    EXPECT_TRUE(hal.isFault());
}

TEST_F(MotorHALTest, Reset_ZerosAllRegisters) {
    registers.encoder_rpm = 1000.0f;
    registers.pwm_duty = 0.5f;
    registers.fault = true;
    
    hal.reset();
    
    EXPECT_FLOAT_EQ(registers.encoder_rpm, 0.0f);
    EXPECT_FLOAT_EQ(registers.pwm_duty, 0.0f);
    EXPECT_FALSE(registers.fault);
}

TEST_F(MotorHALTest, MultipleReads_Consistent) {
    registers.encoder_rpm = 1234.5f;
    EXPECT_FLOAT_EQ(hal.readRPM(), 1234.5f);
    EXPECT_FLOAT_EQ(hal.readRPM(), 1234.5f);
    EXPECT_FLOAT_EQ(hal.readRPM(), 1234.5f);
}

TEST_F(MotorHALTest, WritePWM_BoundaryValues) {
    hal.writePWM(0.0f);
    EXPECT_FLOAT_EQ(registers.pwm_duty, 0.0f);
    
    hal.writePWM(1.0f);
    EXPECT_FLOAT_EQ(registers.pwm_duty, 1.0f);
}
