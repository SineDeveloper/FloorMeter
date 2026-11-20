#ifndef SENSOR_ENCODER_H
#define SENSOR_ENCODER_H

#include <Wire.h>
#include <Arduino.h>
#include "Config.h"

class SensorEncoder {
private:
    const uint8_t AS5600_ADDR = 0x36;    
    const uint8_t RAW_ANGLE_REG = 0x0C;   

    int16_t lastRawAngle = -1;
    int32_t totalPulses = 0;              
    
    float simulatedDistance = 0.0;
    float simulatedSpeed = 0.0;
    unsigned long lastSimTime = 0;

    uint16_t readRawAngle() {
        Wire.beginTransmission(AS5600_ADDR);
        Wire.write(RAW_ANGLE_REG);
        if (Wire.endTransmission() != 0) return 0;
        
        Wire.requestFrom(AS5600_ADDR, (uint8_t)2);
        if (Wire.available() >= 2) {
            uint8_t highByte = Wire.read();
            uint8_t lowByte = Wire.read();
            return ((highByte << 8) | lowByte) & 0x0FFF; 
        }
        return 0;
    }

public:
    void begin() {
        lastRawAngle = readRawAngle();
        totalPulses = 0;
        simulatedDistance = 0.0;
        simulatedSpeed = 0.0;
        lastSimTime = millis();
        Serial.println("[+] Hybrid Encoder Ready.");
    }

    void setSimulatedSpeed(float speedMPS) {
        simulatedSpeed = speedMPS;
    }

    void update() {
        uint16_t currentRawAngle = readRawAngle();
        if (lastRawAngle != -1) {
            int16_t diff = currentRawAngle - lastRawAngle;
            if (diff > 2048) diff -= 4096;
            else if (diff < -2048) diff += 4096;
            totalPulses += diff;
        }
        lastRawAngle = currentRawAngle;

        unsigned long now = millis();
        float dt = (now - lastSimTime) / 1000.0;
        lastSimTime = now;
        if (simulatedSpeed > 0.0) {
            simulatedDistance += simulatedSpeed * dt;
        }
    }

    float getDistanceMeter() {
        float realTurns = (float)totalPulses / 4096.0;
        float wheelCircumferenceMeter = (WHEEL_DIAMETER_MM * 3.14159265) / 1000.0;
        float realDistance = realTurns * wheelCircumferenceMeter;
        return realDistance + simulatedDistance;
    }

    void resetDistance() {
        totalPulses = 0;
        simulatedDistance = 0.0;
        simulatedSpeed = 0.0;
        lastRawAngle = readRawAngle();
        lastSimTime = millis();
        Serial.println("[+] Odometer System Reset.");
    }
};

#endif
