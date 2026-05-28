#pragma once

struct PIDConfig {
    float Kp{0.0f}, Ki{0.0f}, Kd{0.0f}, Ts{0.01f};
    float u_min{0.0f}, u_max{100.0f};
    float integral_max{50.0f};
};

class PIDController {
public:
    explicit PIDController(const PIDConfig& cfg);
    float compute(float setpoint, float measurement);
    void reset();
    [[nodiscard]] float integralState() const noexcept;
    [[nodiscard]] float lastError() const noexcept;

private:
    PIDConfig cfg_;
    float integral_acc_{0.0f};
    float e_prev_{0.0f};
};
