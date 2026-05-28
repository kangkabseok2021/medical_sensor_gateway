#pragma once

struct PIDConfig {
    float Kp, Ki, Kd, Ts;
    float u_min, u_max;
    float integral_max;
};

class PIDController {
public:
    explicit PIDController(PIDConfig cfg);
    float compute(float setpoint, float measurement);
    void reset();
    [[nodiscard]] float integralState() const noexcept;
    [[nodiscard]] float lastError() const noexcept;

private:
    PIDConfig cfg_;
    float integral_acc_{0.0f};
    float e_prev_{0.0f};
};
