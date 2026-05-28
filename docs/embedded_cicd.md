# Embedded CI/CD Pipeline & Automated Hardware Testing

A standalone embedded C++ sub-project showcasing rigorous testing, static analysis, and Continuous Integration (CI) methodologies specifically tailored for embedded software development.

## Core Implementation

This project implements a complete closed-loop motor control system featuring:
- **`IMotorHAL` Interface**: Pure-virtual abstraction for simulated motor hardware.
- **`PIDController`**: Configurable PID control loop featuring clamping and integral anti-windup to prevent unstable accumulation during saturation.
- **`ProcessingLoop`**: High-level application logic driving the control execution based on simulated fault states.

## Testing & Quality Gates

The primary focus of this project is strict QA enforcement mimicking an automotive/aerospace CI pipeline.

### Unit & Integration Testing
- 26 GoogleTests covering basic proportional logic, integral clamping bounds, step response conversions, and integration state resilience against simulated sensor failures.

### Code Quality and Static Analysis Scripts
Robust bash scripts enforce strict rules during CI runs:
- `generate_coverage_report.sh`: Generates line and branch coverage using `gcov` and `lcov`. Fails the pipeline if line coverage falls below **90%**.
- `run_cppcheck.sh`: Triggers immediate failure if any `cppcheck` warnings/errors are detected across the codebase.
- `check_complexity.sh`: Uses `lizard` to compute cyclomatic complexity, failing the pipeline if any function exceeds **M <= 10**.

## Build & Docker Integration

- **Dockerization**: A `Dockerfile` configures a complete toolchain (CMake, gcc-arm, qemu-user-static, clang-tidy, cppcheck, lcov).
- **Docker Compose**: The `docker-compose.yml` abstracts execution environments, allowing identical local and CI execution of three separate layers:
  1. `build-native`: Native x86_64 compile and `ctest`.
  2. `static-analysis`: Enforces formatting, cppcheck, and cyclomatic complexity limits.
  3. `build-arm`: Cross-compiles the C++ core targeting `arm-linux-gnueabihf` and executes the binary locally via `qemu-arm-static`.
