#include "hal/SimulatedMotorHAL.h"
#include <algorithm>

SimulatedMotorHAL::SimulatedMotorHAL(MotorRegisters& registers)
    : registers_(registers) {}

float SimulatedMotorHAL::readRPM() const {
    return registers_.encoder_rpm;
}

void SimulatedMotorHAL::writePWM(float duty) {
    registers_.pwm_duty = std::clamp(duty, 0.0f, 1.0f);
}

bool SimulatedMotorHAL::isFault() const {
    return registers_.fault;
}

void SimulatedMotorHAL::reset() {
    registers_.encoder_rpm = 0.0f;
    registers_.pwm_duty = 0.0f;
    registers_.fault = false;
}
