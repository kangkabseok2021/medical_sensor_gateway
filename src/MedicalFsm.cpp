#include "MedicalFsm.h"

const char* med_state_name(MedState s) noexcept {
    switch (s) {
        case MedState::INITIALISING: return "INITIALISING";
        case MedState::CALIBRATING:  return "CALIBRATING";
        case MedState::MONITORING:   return "MONITORING";
        case MedState::ALARM:        return "ALARM";
        case MedState::ERROR_HALT:   return "ERROR_HALT";
    }
    return "UNKNOWN";
}

bool MedicalFsm::update(const SensorReadings& r) {
    MedState s = state();
    bool alarm_emitted = false;

    switch (s) {
        case MedState::INITIALISING:
            // Self-test is implicit: sensors provided from outside.
            // Gateway calls update() only after selfTest() passes → advance.
            set_state(MedState::CALIBRATING);
            calib_count_ = 0;
            break;

        case MedState::CALIBRATING:
            ++calib_count_;
            if (calib_count_ >= kCalibSamples)
                set_state(MedState::MONITORING);
            break;

        case MedState::MONITORING: {
            // Hard limits — immediate ERROR_HALT.
            if (r.flow_lpm > kFlowHighHard) {
                halt_reason_ = "flow_rate_overdose";
                set_state(MedState::ERROR_HALT);
                alarm_emitted = true;
                break;
            }
            if (r.hr_bpm > kHrHighHard) {
                ++hr_high_count_;
                if (hr_high_count_ >= kHrHighCount) {
                    halt_reason_ = "tachycardia";
                    set_state(MedState::ERROR_HALT);
                    alarm_emitted = true;
                    break;
                }
            } else {
                hr_high_count_ = 0;
            }

            // Soft limits — hysteresis → ALARM.
            bool go_alarm = false;
            if (r.flow_lpm < kFlowLowHard) {
                ++flow_low_count_;
                if (flow_low_count_ >= kFlowAlarmCount)
                    go_alarm = true;
            } else {
                flow_low_count_ = 0;
            }
            if (r.hr_bpm < kHrLowHard) {
                ++hr_low_count_;
                if (hr_low_count_ >= kHrLowCount)
                    go_alarm = true;
            } else {
                hr_low_count_ = 0;
            }
            if (go_alarm) {
                set_state(MedState::ALARM);
                alarm_emitted = true;
            }
            break;
        }

        case MedState::ALARM:
            // Stay in ALARM; consecutive counter incremented per update cycle.
            // acknowledge() clears it; after kMaxAlarms cycles → latch.
            ++consec_alarms_;
            if (consec_alarms_ >= kMaxAlarms) {
                halt_reason_ = "repeated_alarm_without_ack";
                set_state(MedState::ERROR_HALT);
                alarm_emitted = true;
            }
            break;

        case MedState::ERROR_HALT:
            // Latching — only reset() can clear.
            break;
    }

    return alarm_emitted;
}

void MedicalFsm::acknowledge() noexcept {
    if (state() == MedState::ALARM) {
        consec_alarms_ = 0;
        flow_low_count_ = 0;
        hr_low_count_   = 0;
        set_state(MedState::MONITORING);
    }
}

void MedicalFsm::reset() noexcept {
    consec_alarms_  = 0;
    flow_low_count_ = 0;
    hr_low_count_   = 0;
    hr_high_count_  = 0;
    calib_count_    = 0;
    halt_reason_    = "none";
    set_state(MedState::INITIALISING);
}
