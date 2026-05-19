#pragma once
#include "ISensor.h"
#include <random>

// Simulates a cardiac monitor ECG sensor.
// Returns instantaneous heart rate in bpm derived from a synthetic QRS model:
//   H(t) = Σᵢ aᵢ·exp(−(t−tᵢ)²/(2σᵢ²))  at 75 bpm baseline.
// Returned value: heart rate in bpm (smoothed over one RR interval).
class SimulatedECGSensor final : public ISensor {
public:
    explicit SimulatedECGSensor(double baseline_bpm = 75.0,
                                 unsigned seed = 99);

    double           read()              override;
    bool             selfTest()          override { return true; }
    std::string_view name() const noexcept override { return "ECGSensor"; }

    void  set_override(double bpm) noexcept { override_ = bpm; }
    void  clear_override()         noexcept { override_ = -1.0; }

private:
    double                               baseline_bpm_;
    double                               override_{-1.0};
    double                               t_{0.0};
    std::mt19937                         rng_;
    std::normal_distribution<double>     noise_;
};
