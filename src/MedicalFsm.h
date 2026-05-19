#pragma once
#include <atomic>
#include <cstdint>
#include <string_view>

// IEC 60601-class five-state safety FSM.
//
// Transitions:
//   INITIALISING → CALIBRATING  selfTest() passes
//   INITIALISING → ERROR_HALT   selfTest() fails
//   CALIBRATING  → MONITORING   calibration window complete (kCalibSamples)
//   MONITORING   → ALARM        soft limit violated for hysteresis count
//   MONITORING   → ERROR_HALT   hard limit violated immediately
//   ALARM        → MONITORING   operator acknowledgement
//   ALARM        → ERROR_HALT   3 consecutive alarm events without ack
//   ERROR_HALT   → INITIALISING explicit reset (key-switch)
//
// Thread-safe: state readable from alarm publisher thread via std::atomic.
enum class MedState : uint8_t {
    INITIALISING,
    CALIBRATING,
    MONITORING,
    ALARM,
    ERROR_HALT,
};

const char* med_state_name(MedState s) noexcept;

struct SensorReadings {
    double flow_lpm;   // L/min
    double hr_bpm;     // bpm
};

class MedicalFsm {
public:
    // Safety limits (IEC 60601-2-24 / IEC 60601-2-27).
    static constexpr double kFlowLowHard  = 0.1;   // L/min — occlusion
    static constexpr double kFlowHighHard = 15.0;  // L/min — overdose
    static constexpr double kHrLowHard    = 30.0;  // bpm  — bradycardia
    static constexpr double kHrHighHard   = 200.0; // bpm  — tachycardia

    // Hysteresis: consecutive readings required before soft-alarm fires.
    static constexpr int kFlowAlarmCount = 5;   // 5 ms at 1 kHz
    static constexpr int kHrLowCount     = 2000; // 2 s at 1 kHz
    static constexpr int kHrHighCount    = 3000; // 3 s at 1 kHz

    // Calibration: samples collected before entering MONITORING.
    static constexpr int kCalibSamples = 100;

    // Maximum consecutive ALARM events before latching ERROR_HALT.
    static constexpr int kMaxAlarms = 3;

    MedicalFsm() = default;

    // Drive one cycle; call at sensor sample rate.
    // Returns true if an alarm event was emitted this cycle.
    bool update(const SensorReadings& r);

    // Operator acknowledgement — clears ALARM → MONITORING.
    void acknowledge() noexcept;

    // Physical key-switch reset — only way out of ERROR_HALT.
    void reset() noexcept;

    MedState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    // Why ERROR_HALT was entered (populated on transition).
    std::string_view halt_reason() const noexcept { return halt_reason_; }

private:
    void set_state(MedState s) noexcept {
        state_.store(s, std::memory_order_release);
    }

    std::atomic<MedState> state_{MedState::INITIALISING};

    // Hysteresis counters
    int flow_low_count_{0};
    int hr_low_count_{0};
    int hr_high_count_{0};

    // Calibration counter
    int calib_count_{0};

    // Consecutive alarm counter (without ack)
    int consec_alarms_{0};

    const char* halt_reason_{"none"};
};
