#include "ECGSensor.h"
#include <cmath>

// QRS Gaussian parameters — three peaks (Q, R, S)
static constexpr double kA[]     = { -0.1, 1.0, -0.3 };
static constexpr double kTOff[]  = { -0.02, 0.0, 0.02 };  // seconds from R-peak
static constexpr double kSig[]   = { 0.005, 0.008, 0.006 };

static constexpr double kNoiseSigma = 1.5;  // bpm noise
static constexpr double kDt         = 1.0 / 1000.0;

SimulatedECGSensor::SimulatedECGSensor(double baseline_bpm, unsigned seed)
    : baseline_bpm_(baseline_bpm), rng_(seed), noise_(0.0, kNoiseSigma) {}

double SimulatedECGSensor::read() {
    if (override_ >= 0.0)
        return override_;

    double rr_s = 60.0 / baseline_bpm_;
    double phase = std::fmod(t_, rr_s);

    // QRS complex centred at 0.3·RR from start of cycle
    double centre = 0.3 * rr_s;
    double qrs = 0.0;
    for (int i = 0; i < 3; ++i) {
        double dt = phase - centre + kTOff[i];
        qrs += kA[i] * std::exp(-dt * dt / (2.0 * kSig[i] * kSig[i]));
    }

    // Derive instantaneous HR from the QRS amplitude — peaks → 75 bpm baseline
    // Return smooth HR with small noise (physiologically realistic)
    double hr = baseline_bpm_ + qrs * 5.0 + noise_(rng_);
    t_ += kDt;
    return hr;
}
