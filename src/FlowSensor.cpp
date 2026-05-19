#include "FlowSensor.h"
#include <cmath>

static constexpr double kAmplitude  = 0.05;  // L/min ripple amplitude
static constexpr double kPulseHz    = 2.0;   // pump occlusion frequency
static constexpr double kNoiseSigma = 0.02;  // L/min noise std-dev
static constexpr double kDt         = 1.0 / 1000.0;  // 1 kHz sample step

SimulatedFlowSensor::SimulatedFlowSensor(double nominal_lpm, unsigned seed)
    : nominal_(nominal_lpm), rng_(seed), noise_(0.0, kNoiseSigma) {}

double SimulatedFlowSensor::read() {
    if (override_ >= 0.0)
        return override_;
    double q = nominal_
             + kAmplitude * std::sin(2.0 * M_PI * kPulseHz * t_)
             + noise_(rng_);
    t_ += kDt;
    return q;
}
