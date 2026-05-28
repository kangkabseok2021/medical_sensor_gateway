# Embedded Medical Sensor Gateway

A hardware-near **C++17 daemon** that simulates the safety-critical software layer of a patient monitoring device — continuous infusion pump and cardiac monitor. A **HAL abstraction layer** isolates simulated flow and ECG sensors; a **4th-order Butterworth IIR filter** removes measurement noise in real time; a **five-state MedicalFSM** drives the device lifecycle from `INITIALISING` through `MONITORING` to `ALARM` and a latching `ERROR_HALT` when IEC 60601-class thresholds are exceeded. **18 GoogleTests** validate every FSM transition and filter property. A **Python pytest fault-injection suite** starts the compiled daemon as a subprocess and injects out-of-range sensor streams to verify that failsafe logic triggers correctly.

---

## Architecture

```
                      ┌──────────────────────────────────────────┐
                      │  gateway_daemon  (Linux / systemd)        │
                      │                                            │
  ISensor HAL         │   SensorGateway::tick()  (1 kHz timerfd) │
  ┌────────────┐      │     │                                      │
  │ FlowSensor │──────┤     ├─ flow_filter_.tick(raw_flow)         │
  │ ECGSensor  │──────┤     ├─ ecg_filter_.tick(raw_hr)            │
  └────────────┘      │     │                                      │
                      │     ▼                                      │
                      │  MedicalFsm::update(flow_lpm, hr_bpm)     │
                      │     │                                      │
                      │     ├── MONITORING → ALARM (soft limit)    │
                      │     └── MONITORING → ERROR_HALT (hard)     │
                      │                                            │
                      │  AlarmPublisher::emit()                    │
                      │     └── JSON → stdout + named POSIX pipe   │
                      └──────────────────────────────────────────┘
```

### CMake Layer Isolation

| Library | Contents | Depends on |
|---|---|---|
| `hal_lib` | `ISensor`, `FlowSensor`, `ECGSensor`, `ButterworthFilter` | — |
| `gateway_core` | `MedicalFsm`, `SensorGateway`, `AlarmPublisher` | `hal_lib` |
| `gateway_daemon` | `main.cpp`, systemd notify, timerfd loop | `gateway_core` |

Unit tests link only `gateway_core` — no Linux system headers, no timerfd, no systemd required for the test binary.

---

## MedicalFSM

```
              selfTest() fails
              ┌──────────────────────────────────────────────┐
              │                                              │
         INITIALISING ─── selfTest() ok ───► CALIBRATING   │
                                               (100 samples) │
                                                 │           │
                                           calib complete    │
                                                 │           │
              ┌──────────────────── MONITORING ◄─┘           │
              │          ack          │  │                    │
              │     ┌─────────────────┘  │                   │
    soft      ▼     │              hard  ▼                   │
  ALARM ──────────► │         ERROR_HALT ◄────────────────────┘
  (3× without ack)  │          (latching — reset() only)
                    │
            MONITORING (restored)
```

### Safety Thresholds (IEC 60601-2-24 / IEC 60601-2-27)

| Parameter | Soft alarm threshold | Hard halt threshold | Hysteresis |
|---|---|---|---|
| Flow rate | < 0.1 L/min (occlusion) | > 15.0 L/min (overdose) | 5 consecutive readings |
| Heart rate | < 30 bpm (bradycardia) | > 200 bpm (tachycardia) | 2000 / 3000 readings |

Soft-limit alarms require operator acknowledgement (`acknowledge()`). Three consecutive unacknowledged alarms latch `ERROR_HALT`. The only recovery from `ERROR_HALT` is `reset()` — simulating a physical key switch.

---

## Signal Processing

A **4th-order Butterworth low-pass filter** (cutoff 10 Hz, sample rate 1 kHz) runs on every sensor channel:

- Preserves physiologically relevant signals: cardiac (≤ 3 Hz), pump occlusion (≤ 2 Hz)
- Attenuates mains interference (50 / 60 Hz) and mechanical vibration (> 20 Hz)
- Implemented as two cascaded biquad sections in **Direct-Form II transposed** — numerically stable, O(1) per sample
- Coefficients computed analytically via bilinear transform at construction (ported from `cross_platform_signal`)
- Filters are **pre-warmed** (3000 samples at startup) to eliminate IIR startup transients before the FSM begins checking thresholds

---

## Project Layout

## Project Layout

```
medical_sensor_gateway/
├── src/                             [Gateway Daemon]
│   ├── ISensor.h                    Abstract sensor HAL
│   ├── FlowSensor.{h,cpp}           Q(t) = Qₙ + A·sin(2πfₚt) + N(0,σ)
│   ├── ECGSensor.{h,cpp}            QRS Gaussian model, returns HR in bpm
│   ├── ButterworthFilter.{h,cpp}    4th-order IIR, Direct-Form II transposed
│   ├── MedicalFsm.{h,cpp}           5-state safety FSM
│   ├── AlarmPublisher.{h,cpp}       Structured JSON → stdout + POSIX pipe
│   ├── SensorGateway.{h,cpp}        Orchestrator, dependency-injected
│   └── main.cpp                     timerfd 1 kHz loop + systemd notify
├── tests/                           [Gateway Daemon Tests]
│   └── test_gateway.cpp             18 GoogleTests
├── python/                          [Fault Injection]
│   ├── conftest.py                  GatewayFixture + FIFO alarm pipe
│   └── test_fault_injection.py      6 fault-injection tests
├── embedded_cicd_pipeline/          [Embedded CI/CD PID Sub-Project]
│   ├── src/                         PID controller, processing loop, and HAL
│   ├── include/                     Headers
│   ├── tests/                       GoogleTest suite for PID and HAL
│   ├── scripts/                     QA shell scripts (coverage, cppcheck, lizard)
│   ├── CMakeLists.txt               Sub-project CMake config
│   ├── Dockerfile                   Docker container with QEMU ARM setup
│   └── docker-compose.yml           Orchestration for static analysis and ARM build
├── CMakeLists.txt                   Root CMake for Gateway Daemon
├── pyproject.toml                   Python config
└── .github/workflows/ci.yml         GitHub Actions workflow for both projects
```

---

## Quick Start

```bash
# Build (Linux — requires cmake, g++)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j4

# Run 18 GoogleTests
ctest --test-dir build --output-on-failure -V

# Run daemon (normal operation, stdout alarms)
./build/gateway_daemon

# Run daemon with fixed sensor values (fault injection)
./build/gateway_daemon --flow 50.0 --hr 75.0 --alarm-pipe /tmp/alarms.fifo
```

---

## Tests

### C++ — 18 GoogleTests

| Suite | Test | What it verifies |
|---|---|---|
| SensorTest | FlowSensorInDomain | Nominal flow stays positive |
| SensorTest | ECGSensorHeartRateRange | Nominal HR within [40, 150] bpm |
| SensorTest | SelfTestPassesAfterInit | Both sensors pass self-test |
| FilterTest | PassbandPreserved | 1 Hz sine passes with amplitude > 0.9 |
| FilterTest | StopbandAttenuated | 100 Hz sine attenuated to < 0.05 |
| FilterTest | OutputLengthMatchesInput | Vector output length unchanged |
| FilterTest | StepResponseConverges | Step input converges to 1.0 ± 0.01 |
| FilterTest | ZeroPaddingNoThrow | Empty input handled gracefully |
| FsmTest | StartsInInitialising | Initial state is INITIALISING |
| FsmTest | CalibratingOnSelfTestPass | First update → CALIBRATING |
| FsmTest | SelfTestFailureGivesErrorHalt | Failed selfTest() → SensorGateway returns false |
| FsmTest | MonitoringOnCalibrationComplete | CALIBRATING → MONITORING after 100 samples |
| FsmTest | AlarmOnLowFlowFor5Readings | Flow < 0.1 L/min for 5 readings → ALARM |
| FsmTest | ErrorHaltOnOverdoseImmediate | Flow > 15.0 L/min → immediate ERROR_HALT |
| FsmTest | AlarmOnBradycardiaFor2s | HR < 30 bpm for 2000 readings → ALARM |
| FsmTest | ErrorHaltOnTachycardiaFor3s | HR > 200 bpm for 3000 readings → ERROR_HALT |
| FsmTest | ThreeAlarmsWithoutAckGivesErrorHalt | 3 ALARM cycles without ack → ERROR_HALT |
| FsmTest | ResetFromErrorHaltRequiresExplicitCall | Only `reset()` clears ERROR_HALT |

### Python — 6 Fault-Injection Tests

Tests start the compiled `gateway_daemon` binary as a subprocess with pinned sensor values, then read alarm events from a named POSIX pipe:

| Test | Condition | Expected event |
|---|---|---|
| test_overdose_triggers_error_halt | flow = 50.0 L/min | ERROR_HALT (trigger: flow_rate_overdose) |
| test_occlusion_triggers_alarm | flow = 0.0 L/min | ALARM or ERROR_HALT |
| test_three_alarms_latch_error_halt | flow = 0.0 L/min, no ack | ERROR_HALT |
| test_tachycardia_triggers_error_halt | hr = 300 bpm | ERROR_HALT (trigger: tachycardia) |
| test_normal_range_no_alarm | flow = 1.0, hr = 75 | no event for 0.4 s |
| test_bradycardia_triggers_alarm | hr = 20 bpm | ALARM or ERROR_HALT |

---

## CI

| Job | Runner | What it does |
|---|---|---|
| `lint` | ubuntu-latest | ruff check + ruff format on Python |
| `cpp-build-test` | ubuntu-latest | CMake build + 18 GoogleTests via ctest |
| `python-fault-injection` | ubuntu-latest | Build daemon, run 6 subprocess fault-injection tests |
| `arm64-cross-build` | ubuntu-latest | Cross-compile `gateway_core` for aarch64 (no libmodbus needed) |
| `embedded-cicd-native-build` | ubuntu-latest | Native C++ build and test for the `embedded_cicd_pipeline` project via Docker compose |
| `embedded-cicd-static-analysis` | ubuntu-latest | Run `clang-tidy`, `cppcheck`, and `lizard` on the `embedded_cicd_pipeline` codebase |
| `embedded-cicd-coverage-gate` | ubuntu-latest | Run `lcov` to enforce >90% code coverage on `embedded_cicd_pipeline` |
| `embedded-cicd-arm-cross` | ubuntu-latest | Cross-compile `embedded_cicd_pipeline` and execute tests via `qemu-arm-static` |

---

## Tech Stack

| Layer | Technology |
|---|---|
| C++ daemon | C++17, CMake, systemd notify |
| Sensor model | timerfd 1 kHz loop, POSIX, Gaussian noise |
| Signal processing | Butterworth IIR (Direct-Form II transposed), bilinear transform |
| Safety FSM | IEC 60601-2-24 / IEC 60601-2-27 thresholds, std::atomic state |
| Alarm output | Structured JSON, named POSIX pipe |
| Unit tests | GoogleTest (18 tests, TDD) |
| Fault injection | Python 3.12+, pytest, subprocess, select |
| Targets | x86\_64 (CI/dev), ARM64 (Raspberry Pi 4 / i.MX 8M Plus) |
