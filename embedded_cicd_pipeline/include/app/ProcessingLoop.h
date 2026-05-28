#pragma once
#include <cstdint>
#include "hal/IMotorHAL.h"
#include "pid/PIDController.h"

class ProcessingLoop {
public:
    ProcessingLoop(IMotorHAL& hal, PIDController& pid, float setpoint_rpm);
    void tick();
    void setSetpoint(float rpm);
    [[nodiscard]] float lastOutput() const noexcept;
    [[nodiscard]] uint64_t tickCount() const noexcept;

private:
    IMotorHAL& hal_;
    PIDController& pid_;
    float setpoint_rpm_;
    float last_output_{0.0f};
    uint64_t tick_count_{0};
};
