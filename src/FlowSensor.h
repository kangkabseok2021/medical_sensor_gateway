#pragma once
#include "ISensor.h"
#include <random>

// Simulates a peristaltic infusion pump flow sensor.
// Q(t) = Q_nominal + A·sin(2π·f_pulse·t) + N(0, σ_Q)
// f_pulse = 2 Hz (pump occlusion frequency), σ_Q = 0.02 L/min.
class SimulatedFlowSensor final : public ISensor {
public:
    // nominal_lpm: baseline flow in L/min (e.g. 1.0 for normal infusion).
    explicit SimulatedFlowSensor(double nominal_lpm = 1.0,
                                  unsigned seed = 42);

    double           read()              override;
    bool             selfTest()          override { return true; }
    std::string_view name() const noexcept override { return "FlowSensor"; }

    // Override reading for test injection.
    void  set_override(double lpm) noexcept { override_ = lpm; }
    void  clear_override()         noexcept { override_ = -1.0; }

private:
    double                               nominal_;
    double                               override_{-1.0};
    double                               t_{0.0};
    std::mt19937                         rng_;
    std::normal_distribution<double>     noise_;
};
