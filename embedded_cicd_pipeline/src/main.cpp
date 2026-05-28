#include <iostream>
#include "hal/SimulatedMotorHAL.h"
#include "pid/PIDController.h"
#include "app/ProcessingLoop.h"

int main() {
    MotorRegisters registers;
    SimulatedMotorHAL hal(registers);
    
    PIDConfig config{
        0.001f, // Kp
        0.05f,  // Ki
        0.0001f, // Kd
        0.001f, // Ts (1 ms)
        0.0f,   // u_min
        1.0f,   // u_max
        1000.0f // integral_max
    };
    PIDController pid(config);
    
    ProcessingLoop loop(hal, pid, 3000.0f);
    
    // Simulate first-order motor dynamics: encoder_rpm += Ts*(K_motor*pwm_duty - encoder_rpm)/tau_motor
    float K_motor = 5000.0f;
    float tau_motor = 0.5f;
    
    for (int i = 0; i < 5000; ++i) {
        loop.tick();
        
        float derivative = (K_motor * registers.pwm_duty - registers.encoder_rpm) / tau_motor;
        registers.encoder_rpm += config.Ts * derivative;
        
        if (i % 500 == 0) {
            std::cout << "Tick: " << i 
                      << ", RPM: " << registers.encoder_rpm 
                      << ", PWM: " << registers.pwm_duty << std::endl;
        }
    }
    
    return 0;
}
