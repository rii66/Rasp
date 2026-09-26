#ifndef PATRI_PID_H
#define PATRI_PID_H

// PID internal
extern float pidError;
extern float pidIntegral;
extern float pidDerivative;
extern float lastError;

// Target
extern int targetTemp;

// API
int readTemp();
void detectTip();
void updatePID();
void handleSafety();

#endif