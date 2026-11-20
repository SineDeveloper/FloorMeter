#include <Wire.h>
#include "Config.h"
#include "StorageManager.h"
#include "HttpServerManager.h"
#include "SensorEncoder.h"
#include "SensorIMU.h"

StorageManager storage;
HttpServerManager network;
SensorEncoder encoder;
SensorIMU imu;

bool isMeasuring = false;    
float globalDistance = 0.0;
float globalPitch = 0.0;
float globalRoll = 0.0;

unsigned long simulationStartTime = 0;
bool isSimulatingRun = false;
unsigned long timeOffset = 0; 

void resetDataSystem() {
    storage.clearData();     
    encoder.resetDistance(); 
    timeOffset = millis();   
    Serial.println("[+] Session fully initialized. Ready for new recording.");
}

void triggerIMUCalibrate() {
    imu.calibrate();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Super Flat Floor Meter [VERIFIED BASELINE] ===");

    Wire.begin(21, 22);
    Wire.setClock(400000); 

    if (storage.begin()) {
        storage.initLogFile();
    }

    network.begin();
    encoder.begin();
    
    if (!imu.begin()) {
        Serial.println("[-] Hardware Error: IMU not found!");
    }
    timeOffset = millis(); 
    Serial.println("[+] System Ready.");
}

void loop() {
    network.handleClient();
    encoder.update(); 
    imu.update();

    if (Serial.available() > 0) {
        float speedInput = Serial.parseFloat(); 
        if (speedInput > 0.0) {
            Serial.printf("[SIM] Auto-Initializing New 10-Second Run at: %.2f m/s\n", speedInput);
            resetDataSystem(); 
            encoder.setSimulatedSpeed(speedInput);
            simulationStartTime = millis();
            isSimulatingRun = true;
            isMeasuring = true; 
        }
    }

    if (isSimulatingRun && (millis() - simulationStartTime >= 10000)) {
        Serial.println("[SIM] 10 Seconds complete. Stopped.");
        encoder.setSimulatedSpeed(0.0);
        isSimulatingRun = false;
        isMeasuring = false; 
    }

    globalDistance = encoder.getDistanceMeter();
    globalPitch = imu.getPitch();
    globalRoll = imu.getRoll();

    static unsigned long lastLog = 0;
    if (isMeasuring && (millis() - lastLog >= 300)) {
        lastLog = millis();

        String safetyText = "NORMAL"; 
        float absP = fabs(globalPitch);
        float absR = fabs(globalRoll);
        
        if (absR >= 0.5 || absP >= 1.0) safetyText = "DANGER"; 
        else if (absR >= 0.3 || absP >= 0.6) safetyText = "WARNING"; 

        uint32_t relativeTimestamp = millis() - timeOffset;

        Serial.printf("LOGGING,Time:%u,Dist:%.3f,Pitch:%.1f,Roll:%.1f,Status:%s\n", 
                      relativeTimestamp, globalDistance, globalPitch, globalRoll, safetyText.c_str());
        
        storage.logData(relativeTimestamp, globalDistance, globalPitch, globalRoll, safetyText);
    }
}
