#pragma once

class IMotorHAL {
public:
    virtual ~IMotorHAL() = default;
    [[nodiscard]] virtual float readRPM() const = 0;
    virtual void writePWM(float duty) = 0;
    [[nodiscard]] virtual bool isFault() const = 0;
    virtual void reset() = 0;
};
