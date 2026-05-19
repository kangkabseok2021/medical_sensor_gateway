#pragma once
#include <string_view>

// Abstract hardware abstraction layer for a medical sensor.
// Implementations: SimulatedFlowSensor, SimulatedECGSensor, MockSensor.
class ISensor {
public:
    virtual ~ISensor() = default;

    // Return the current measurement in physical units.
    virtual double read() = 0;

    // Hardware self-test — true = pass, false = fail → ERROR_HALT.
    virtual bool selfTest() = 0;

    virtual std::string_view name() const noexcept = 0;
};
