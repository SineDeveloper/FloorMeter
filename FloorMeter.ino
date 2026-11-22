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

// ---- 3-point geometry: live wheel heights (mm, relative to body center) ---
float g_frontTrackMm = DEFAULT_FRONT_TRACK_MM;
float g_rearArmMm    = DEFAULT_REAR_ARM_MM;
float g_sectionMm    = DEFAULT_SECTION_MM;
float g_zLeftMm  = 0.0;
float g_zRightMm = 0.0;
float g_zRearMm  = 0.0;

// ---- Section accumulation state (one rolling summary per SECTION_MM) ------
int   secCurrent = -1;      // index of the section being accumulated
float secSumP = 0.0, secSumR = 0.0;
float secSumL = 0.0, secSumRmm = 0.0, secSumRear = 0.0;
float secMinL = 0.0, secMaxL = 0.0;
float secMinR = 0.0, secMaxR = 0.0;
float secMinRear = 0.0, secMaxRear = 0.0;
int   secCount = 0;
float secEndDist = 0.0;

unsigned long simulationStartTime = 0;
bool isSimulatingRun = false;
unsigned long timeOffset = 0; 

// Instantaneous wheel heights from IMU angles (mm, relative to body center).
void computeWheelHeights() {
    float rollRad  = globalRoll  * PI / 180.0;
    float pitchRad = globalPitch * PI / 180.0;
    g_zLeftMm  = SIGN_ROLL_LEFT  * (g_frontTrackMm * 0.5f) * tan(rollRad);
    g_zRightMm = SIGN_ROLL_RIGHT * (g_frontTrackMm * 0.5f) * tan(rollRad);
    g_zRearMm  = SIGN_PITCH_REAR * g_rearArmMm * tan(pitchRad);
}

void resetSectionTracker() {
    secCurrent = -1;
    secSumP = secSumR = 0.0;
    secSumL = secSumRmm = secSumRear = 0.0;
    secMinL = secMinR = secMinRear = 0.0;
    secMaxL = secMaxR = secMaxRear = 0.0;
    secCount = 0;
    secEndDist = 0.0;
}

// Write the SECTION summary row for the section that just finished.
void finalizeSection() {
    if (secCount == 0) return;
    float inv = 1.0f / secCount;
    storage.logSection(secCurrent, secEndDist,
                       secSumP * inv, secSumR * inv,
                       secSumL * inv, secSumRmm * inv, secSumRear * inv,
                       secCount,
                       secMaxL - secMinL, secMaxR - secMinR, secMaxRear - secMinRear);
    Serial.printf("[SEC] Section %d done: Z_L=%d Z_R=%d Z_Rear=%d mm (%d samples)\n",
                  secCurrent, (int)lroundf(secSumL * inv), (int)lroundf(secSumRmm * inv),
                  (int)lroundf(secSumRear * inv), secCount);
}

// Fold one sample into the current section; emit a SECTION row per boundary crossed.
void updateSectionTracker(float distanceM, float pitch, float roll,
                          float zL, float zR, float zRear) {
    int cur = (int)(distanceM * 1000.0f / g_sectionMm);
    if (cur > secCurrent) {
        if (secCurrent >= 0) finalizeSection();
        secCurrent = cur;
        secSumP = secSumR = 0.0;
        secSumL = secSumRmm = secSumRear = 0.0;
        secMinL = secMaxL = zL;
        secMinR = secMaxR = zR;
        secMinRear = secMaxRear = zRear;
        secCount = 0;
    }
    secSumP += pitch;
    secSumR += roll;
    secSumL += zL;
    secSumRmm += zR;
    secSumRear += zRear;
    if (zL < secMinL) secMinL = zL;   if (zL > secMaxL) secMaxL = zL;
    if (zR < secMinR) secMinR = zR;   if (zR > secMaxR) secMaxR = zR;
    if (zRear < secMinRear) secMinRear = zRear;
    if (zRear > secMaxRear) secMaxRear = zRear;
    secCount++;
    secEndDist = distanceM;
}

void resetDataSystem() {
    storage.clearData();     
    encoder.resetDistance(); 
    timeOffset = millis();   
    resetSectionTracker();
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
        // Restore persisted geometry (defaults are applied + written on first boot).
        storage.loadConfig(&g_frontTrackMm, &g_rearArmMm, &g_sectionMm);
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

    // Live wheel heights (mm) — always kept fresh for the web dashboard.
    computeWheelHeights();

    static unsigned long lastLog = 0;
    if (isMeasuring && (millis() - lastLog >= 300)) {
        lastLog = millis();

        String safetyText = "NORMAL"; 
        float absP = fabs(globalPitch);
        float absR = fabs(globalRoll);
        
        if (absR >= 0.5 || absP >= 1.0) safetyText = "DANGER"; 
        else if (absR >= 0.3 || absP >= 0.6) safetyText = "WARNING"; 

        uint32_t relativeTimestamp = millis() - timeOffset;

        float distCm = globalDistance * 100.0f;
        if (distCm < 0.0f) distCm = 0.0f;
        Serial.printf("LOGGING,Time:%u,Dist:%ld cm,P:%.1f,R:%.1f,Z_L:%d mm,Z_R:%d mm,Z_Rear:%d mm,Status:%s\n",
                      relativeTimestamp, (long)(distCm + 0.5f), globalPitch, globalRoll,
                      (int)lroundf(g_zLeftMm), (int)lroundf(g_zRightMm), (int)lroundf(g_zRearMm),
                      safetyText.c_str());

        storage.logSample(relativeTimestamp, secCurrent < 0 ? 0 : secCurrent, globalDistance,
                          globalPitch, globalRoll, g_zLeftMm, g_zRightMm, g_zRearMm, safetyText);
        updateSectionTracker(globalDistance, globalPitch, globalRoll,
                             g_zLeftMm, g_zRightMm, g_zRearMm);
    }
}
