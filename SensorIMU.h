#ifndef SENSOR_IMU_H
#define SENSOR_IMU_H

#include <Wire.h>
#include <Arduino.h>

class SensorIMU {
private:
    const uint8_t MPU_ADDR = 0x68; 
    const uint8_t PWR_MGMT_1 = 0x6B;
    const uint8_t ACCEL_XOUT_H = 0x3B;
    
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    
    float gx_offset = 0.0, gy_offset = 0.0;
    float pitch = 0.0;
    float roll = 0.0;
    unsigned long lastTime = 0;

    static const int FILTER_SIZE = 10;
    float pitchBuffer[FILTER_SIZE] = {0};
    float rollBuffer[FILTER_SIZE] = {0};
    int bufferIndex = 0;

    void addBuffer(float p, float r) {
        pitchBuffer[bufferIndex] = p;
        rollBuffer[bufferIndex] = r;
        bufferIndex = (bufferIndex + 1) % FILTER_SIZE;
    }

    float getAveragePitch() {
        float sum = 0;
        for(int i = 0; i < FILTER_SIZE; i++) sum += pitchBuffer[i];
        return sum / FILTER_SIZE;
    }

    float getAverageRoll() {
        float sum = 0;
        for(int i = 0; i < FILTER_SIZE; i++) sum += rollBuffer[i];
        return sum / FILTER_SIZE;
    }

public:
    bool begin() {
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(PWR_MGMT_1);
        Wire.write(0); 
        if (Wire.endTransmission() != 0) {
            Serial.println("[-] MPU9250 Connection Failed.");
            return false;
        }
        
        Serial.println("[+] MPU9250 Connected.");
        calibrate();
        lastTime = millis();
        return true;
    }

    void calibrate() {
        Serial.println("[...] Calibrating Gyro offsets... Keep still.");
        long sum_gx = 0, sum_gy = 0;
        int samples = 300; 
        
        for (int i = 0; i < samples; i++) {
            readRawData();
            sum_gx += gx;
            sum_gy += gy;
            delay(4);
        }
        
        gx_offset = (float)sum_gx / samples;
        gy_offset = (float)sum_gy / samples;
        
        for(int i=0; i<FILTER_SIZE; i++) {
            pitchBuffer[i] = 0.0;
            rollBuffer[i] = 0.0;
        }
        Serial.println("[+] Gyro calibration complete.");
    }

    void readRawData() {
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(ACCEL_XOUT_H);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, (uint8_t)14); 
        
        if (Wire.available() >= 14) {
            ax = (Wire.read() << 8) | Wire.read();
            ay = (Wire.read() << 8) | Wire.read();
            az = (Wire.read() << 8) | Wire.read();
            int16_t temp = (Wire.read() << 8) | Wire.read(); 
            gx = (Wire.read() << 8) | Wire.read();
            gy = (Wire.read() << 8) | Wire.read();
            gz = (Wire.read() << 8) | Wire.read();
        }
    }

    void update() {
        readRawData();
        
        unsigned long currentTime = millis();
        float dt = (currentTime - lastTime) / 1000.0; 
        if (dt <= 0.0) dt = 0.001; 
        lastTime = currentTime;
        
        float gyroXrate = (gx - gx_offset) / 131.0;
        float gyroYrate = (gy - gy_offset) / 131.0;
        
        float accelPitch = atan2((float)ay, sqrt((float)ax*ax + (float)az*az)) * 180.0 / PI;
        float accelRoll  = atan2((float)-ax, (float)az) * 180.0 / PI;
        
        pitch = 0.998 * (pitch + gyroXrate * dt) + 0.002 * accelPitch;
        roll  = 0.998 * (roll + gyroYrate * dt) + 0.002 * accelRoll;

        addBuffer(pitch, roll);
    }

    float getPitch() { 
        float avgP = getAveragePitch();
        if (abs(avgP) < 0.1) return 0.0; 
        return round(avgP * 10.0) / 10.0; 
    }
    
    float getRoll()  { 
        float avgR = getAverageRoll();
        if (abs(avgR) < 0.1) return 0.0;
        return round(avgR * 10.0) / 10.0; 
    }
};

#endif
