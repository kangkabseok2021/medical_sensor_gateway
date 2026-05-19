#pragma once
#include "AlarmPublisher.h"
#include "ButterworthFilter.h"
#include "ISensor.h"
#include "MedicalFsm.h"
#include <memory>

// Orchestrates one deterministic cycle:
//   ISensor::read() → ButterworthFilter::tick() → MedicalFsm::update()
//                   → AlarmPublisher::emit() on state change.
//
// Both ISensor instances and MedicalFsm are injected — fully mockable.
class SensorGateway {
public:
    SensorGateway(std::shared_ptr<ISensor>    flow_sensor,
                  std::shared_ptr<ISensor>    ecg_sensor,
                  std::shared_ptr<MedicalFsm> fsm,
                  AlarmPublisher*             publisher);

    // Run sensor self-tests.  Returns false if any sensor fails.
    bool self_test();

    // Drive one 1 ms cycle.  Call from timerfd loop.
    void tick();

    MedState state() const noexcept { return fsm_->state(); }

private:
    std::shared_ptr<ISensor>    flow_;
    std::shared_ptr<ISensor>    ecg_;
    std::shared_ptr<MedicalFsm> fsm_;
    AlarmPublisher*             pub_;

    ButterworthFilter           flow_filter_;
    ButterworthFilter           ecg_filter_;

    MedState                    last_state_{MedState::INITIALISING};
};
