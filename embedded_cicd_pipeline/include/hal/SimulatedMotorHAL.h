#pragma once
#include "IMotorHAL.h"

struct MotorRegisters {
    float encoder_rpm{0.0f};
    float pwm_duty{0.0f};
    bool fault{false};
};

class SimulatedMotorHAL : public IMotorHAL {
public:
    explicit SimulatedMotorHAL(MotorRegisters& registers);
    [[nodiscard]] float readRPM() const override;
    void writePWM(float duty) override;
    [[nodiscard]] bool isFault() const override;
    void reset() override;

private:
    MotorRegisters& registers_;
};
