#include "SensorGateway.h"

// 4th-order Butterworth @ 10 Hz cutoff, 1 kHz sample rate.
// Preserves cardiac (≤3 Hz) and pump (≤2 Hz) signals; rejects mains 50/60 Hz.
static constexpr double kCutoffHz    = 10.0;
static constexpr double kSampleRateHz = 1000.0;

SensorGateway::SensorGateway(std::shared_ptr<ISensor>    flow_sensor,
                              std::shared_ptr<ISensor>    ecg_sensor,
                              std::shared_ptr<MedicalFsm> fsm,
                              AlarmPublisher*             publisher)
    : flow_(std::move(flow_sensor))
    , ecg_(std::move(ecg_sensor))
    , fsm_(std::move(fsm))
    , pub_(publisher)
    , flow_filter_(kCutoffHz, kSampleRateHz)
    , ecg_filter_(kCutoffHz, kSampleRateHz)
{}

// Filter warmup samples: 3000 × (1/1 kHz) = 3 s — enough for the
// 4th-order Butterworth (dominant τ ≈ 260 ms) to converge to < 1 % error.
// This eliminates startup transients before the FSM begins checking thresholds.
static constexpr int kWarmupSamples = 3000;

bool SensorGateway::self_test() {
    if (!flow_->selfTest() || !ecg_->selfTest()) return false;
    double f0 = flow_->read();
    double e0 = ecg_->read();
    for (int i = 0; i < kWarmupSamples; ++i) {
        flow_filter_.tick(f0);
        ecg_filter_.tick(e0);
    }
    return true;
}

void SensorGateway::tick() {
    double flow_raw = flow_->read();
    double ecg_raw  = ecg_->read();

    SensorReadings r{
        .flow_lpm = flow_filter_.tick(flow_raw),
        .hr_bpm   = ecg_filter_.tick(ecg_raw),
    };

    bool alarm = fsm_->update(r);
    MedState cur = fsm_->state();

    if (cur != last_state_ || alarm) {
        if (pub_) {
            if (cur == MedState::ALARM || cur == MedState::ERROR_HALT) {
                const char* trigger = fsm_->halt_reason().data();
                pub_->emit(cur, trigger, r.flow_lpm, "L/min");
            }
        }
        last_state_ = cur;
    }
}
