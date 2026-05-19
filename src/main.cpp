#include "AlarmPublisher.h"
#include "ECGSensor.h"
#include "FlowSensor.h"
#include "MedicalFsm.h"
#include "SensorGateway.h"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/timerfd.h>
#include <unistd.h>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

static volatile sig_atomic_t g_stop = 0;
static void on_signal(int) { g_stop = 1; }

int main(int argc, char* argv[]) {
    std::string alarm_pipe;
    double  fixed_flow = -1.0;  // < 0 = use simulated sensor
    double  fixed_hr   = -1.0;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--alarm-pipe") == 0 && i + 1 < argc)
            alarm_pipe = argv[++i];
        if (std::strcmp(argv[i], "--flow") == 0 && i + 1 < argc)
            fixed_flow = std::atof(argv[++i]);
        if (std::strcmp(argv[i], "--hr") == 0 && i + 1 < argc)
            fixed_hr = std::atof(argv[++i]);
    }

    std::signal(SIGTERM, on_signal);
    std::signal(SIGINT,  on_signal);

    auto flow_sensor = std::make_shared<SimulatedFlowSensor>(
        fixed_flow > 0.0 ? fixed_flow : 1.0);
    auto ecg_sensor = std::make_shared<SimulatedECGSensor>(
        fixed_hr > 0.0 ? fixed_hr : 75.0);

    // --flow / --hr pin the sensor to an exact value (no noise) for testing.
    if (fixed_flow >= 0.0) flow_sensor->set_override(fixed_flow);
    if (fixed_hr   >= 0.0) ecg_sensor->set_override(fixed_hr);

    auto fsm = std::make_shared<MedicalFsm>();
    AlarmPublisher pub(alarm_pipe);

    SensorGateway gw(
        std::static_pointer_cast<ISensor>(flow_sensor),
        std::static_pointer_cast<ISensor>(ecg_sensor),
        fsm, &pub);

    if (!gw.self_test()) {
        std::cerr << "Sensor self-test failed\n";
        return 1;
    }

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd < 0) { perror("timerfd_create"); return 1; }

    itimerspec its{};
    its.it_value.tv_nsec    = 1'000'000;
    its.it_interval.tv_nsec = 1'000'000;
    timerfd_settime(tfd, 0, &its, nullptr);

#ifdef HAVE_SYSTEMD
    sd_notify(0, "READY=1");
#endif

    uint64_t exp = 0;
    while (!g_stop) {
        if (read(tfd, &exp, sizeof(exp)) < 0) break;
#ifdef HAVE_SYSTEMD
        sd_notify(0, "WATCHDOG=1");
#endif
        gw.tick();
    }

    close(tfd);
    return 0;
}
