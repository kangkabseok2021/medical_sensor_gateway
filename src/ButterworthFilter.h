#pragma once
#include <vector>

// 4th-order Butterworth low-pass IIR filter — Direct-Form II transposed.
// Coefficients computed analytically via bilinear transform at construction.
// Ported from cross_platform_signal; decoupled from ISignalFilter interface.
class ButterworthFilter {
public:
    // cutoff_hz: -3 dB cutoff. sample_rate: Fs in Hz.
    ButterworthFilter(double cutoff_hz, double sample_rate);

    // Filter a single sample (stateful).
    double tick(double x);

    // Filter a vector (convenience).
    std::vector<double> apply(const std::vector<double>& in);

    void reset_state() noexcept;

private:
    struct Biquad {
        double b0, b1, b2;
        double a1, a2;
        double w1{0}, w2{0};
        double tick(double x);
    };

    Biquad sec1_, sec2_;
};
