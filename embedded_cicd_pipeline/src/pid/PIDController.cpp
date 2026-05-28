#include "pid/PIDController.h"
#include <algorithm>

PIDController::PIDController(const PIDConfig& cfg) : cfg_(cfg) {}

float PIDController::compute(float setpoint, float measurement) {
    float e = setpoint - measurement;
    
    integral_acc_ += e * cfg_.Ts;
    integral_acc_ = std::clamp(integral_acc_, -cfg_.integral_max, cfg_.integral_max);
    
    float derivative = (e - e_prev_) / cfg_.Ts;
    float u = cfg_.Kp * e + cfg_.Ki * integral_acc_ + cfg_.Kd * derivative;
    
    e_prev_ = e;
    
    return std::clamp(u, cfg_.u_min, cfg_.u_max);
}

void PIDController::reset() {
    integral_acc_ = 0.0f;
    e_prev_ = 0.0f;
}

float PIDController::integralState() const noexcept {
    return integral_acc_;
}

float PIDController::lastError() const noexcept {
    return e_prev_;
}
