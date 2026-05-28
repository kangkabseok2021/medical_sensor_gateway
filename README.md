# Medical Sensor Gateway & Embedded CI/CD Pipeline

Two embedded systems projects in one repository — sharing a C++17 architecture, safety-critical testing methodologies, and a unified GitHub Actions pipeline.

| Project | Description | Docs |
|---|---|---|
| **Gateway Daemon** | C++17 sensor simulation daemon + 4th-order Butterworth filters + Medical safety FSM + Python fault injection | [docs/gateway.md](docs/gateway.md) |
| **Embedded CI/CD PID** | C++17 PID closed-loop motor control + QEMU ARM cross-compilation + Strict static analysis & coverage gates | [docs/embedded_cicd.md](docs/embedded_cicd.md) |

---

## Repository Layout

```
medical_sensor_gateway/
├── src/                             Gateway Daemon C++ project (FSM + HAL + Filter)
├── tests/                           Gateway Daemon 18 GoogleTests
├── python/                          Gateway Daemon Python fault injection simulator
├── embedded_cicd_pipeline/          Embedded CI/CD CMake project (PID + HAL + QA Scripts)
│   ├── src/                         PID controller, processing loop, and HAL
│   ├── tests/                       GoogleTest suite for PID and HAL
│   ├── scripts/                     QA shell scripts (coverage, cppcheck, lizard)
│   ├── Dockerfile                   Docker container with QEMU ARM setup
│   └── docker-compose.yml           Orchestration for static analysis and ARM build
├── docs/
│   ├── gateway.md                   Gateway system documentation
│   └── embedded_cicd.md             Embedded CI/CD documentation
├── CMakeLists.txt                   Root CMake for Gateway Daemon
├── pyproject.toml                   Python config
└── .github/workflows/ci.yml         GitHub Actions workflow for both projects
```

---

## Quick Start

```bash
# Gateway Daemon
cmake -B build_daemon -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build_daemon -j4
ctest --test-dir build_daemon --output-on-failure
./build_daemon/gateway_daemon --flow 50.0 --hr 75.0 --alarm-pipe /tmp/alarms.fifo

# Embedded CI/CD Pipeline
cd embedded_cicd_pipeline
docker compose run build-native
docker compose run static-analysis
docker compose run build-arm
```

---

## CI

Eight jobs on every push to `main` validating both projects independently:

| Job | What it validates |
|---|---|
| `lint` | ruff check + ruff format on Gateway Python fault injection |
| `cpp-build-test` | Gateway Daemon C++ build + 18 GoogleTests via ctest |
| `python-fault-injection` | Gateway Daemon 6 subprocess fault-injection tests |
| `arm64-cross-build` | Cross-compile `gateway_core` for aarch64 (no tests) |
| `embedded-cicd-native-build` | Native C++ build and 26 GoogleTests for `embedded_cicd_pipeline` |
| `embedded-cicd-static-analysis` | `clang-tidy`, `cppcheck`, and `lizard` limits on `embedded_cicd_pipeline` |
| `embedded-cicd-coverage-gate` | `lcov` enforces >90% code coverage on `embedded_cicd_pipeline` |
| `embedded-cicd-arm-cross` | Cross-compile `embedded_cicd_pipeline` and execute tests via `qemu-arm-static` |
