#ifndef AIR_PID_H
#define AIR_PID_H

#include <Arduino.h>

class PIDController {
public:
  float kp = 12.0f;
  float ki = 0.6f;
  float kd = 25.0f;

  void reset() {
    integral = 0;
    prev_error = 0;
  }

  float compute(float setpoint, float current, float dt) {
    float error = setpoint - current;

    // Anti-windup
    integral += error * dt;
    integral = constrain(integral, -200.0f, 200.0f);

    float derivative = (error - prev_error) / dt;
    prev_error = error;

    float output = (kp * error) + (ki * integral) + (kd * derivative);

    // Tapering saat mendekati setpoint
    float taper_start = setpoint * 0.85f;
    if (current > taper_start && setpoint > taper_start) {
      float scale = (setpoint - current) / (setpoint - taper_start);
      scale = constrain(scale, 0.0f, 1.0f);
      output *= scale;
    }

    return constrain(output, 0.0f, 100.0f);
  }

  void setTunings(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
  }

private:
  float integral = 0;
  float prev_error = 0;
};

#endif