#include "app/ProcessingLoop.h"

ProcessingLoop::ProcessingLoop(IMotorHAL& hal, PIDController& pid, float setpoint_rpm)
    : hal_(hal), pid_(pid), setpoint_rpm_(setpoint_rpm) {}

void ProcessingLoop::tick() {
    if (hal_.isFault()) {
        hal_.writePWM(0.0f);
        last_output_ = 0.0f;
        return;
    }
    
    float rpm = hal_.readRPM();
    float u = pid_.compute(setpoint_rpm_, rpm);
    hal_.writePWM(u);
    last_output_ = u;
    ++tick_count_;
}

void ProcessingLoop::setSetpoint(float rpm) {
    setpoint_rpm_ = rpm;
}

float ProcessingLoop::lastOutput() const noexcept {
    return last_output_;
}

uint64_t ProcessingLoop::tickCount() const noexcept {
    return tick_count_;
}
