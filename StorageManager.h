#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <LittleFS.h>
#include "Config.h"

class StorageManager {
private:
    // Pull a named float out of a small, writer-controlled JSON document.
    // Returns `def` when the key is absent or unparseable. No JSON library needed.
    static float jsonFloat(const String& s, const char* key, float def) {
        String pat = "\"";
        pat += key;
        pat += "\"";
        int i = s.indexOf(pat);
        if (i < 0) return def;
        int c = s.indexOf(':', i);
        if (c < 0) return def;
        int j = c + 1;
        while (j < s.length() && (s.charAt(j) == ' ' || s.charAt(j) == '\t')) j++;
        int start = j;
        while (j < s.length()) {
            char ch = s.charAt(j);
            if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.' || ch == 'e' || ch == 'E') j++;
            else break;
        }
        if (j == start) return def;
        float v = s.substring(start, j).toFloat();
        return (v == 0.0f && def != 0.0f) ? def : v;
    }

public:
    bool begin() {
        if (!LittleFS.begin(true)) { 
            Serial.println("[-] LittleFS Mount Failed");
            return false;
        }
        Serial.println("[+] LittleFS Mounted Successfully");
        return true;
    }

    void initLogFile() {
        File file = LittleFS.open(DATA_FILE, FILE_WRITE);
        if (!file) {
            Serial.println("[-] Failed to open file for writing");
            return;
        }
        file.println("Row_Type,Section,Timestamp(ms),Distance(m),Pitch(deg),Roll(deg),Z_Left(mm),Z_Right(mm),Z_Rear(mm),Samples,Range_Left(mm),Range_Right(mm),Range_Rear(mm),Safety_Status");
        file.close();
        Serial.println("[+] Data file initialized with headers.");
    }

    void clearData() {
        if (LittleFS.exists(DATA_FILE)) {
            LittleFS.remove(DATA_FILE);
            initLogFile();
            Serial.println("[+] Data file cleared.");
        }
    }

    // Per-sample row (one every log interval while measuring).
    void logSample(uint32_t timestamp, int section, float distance,
                   float pitch, float roll,
                   float zLeft, float zRight, float zRear,
                   const String& safetyStatus) {
        File file = LittleFS.open(DATA_FILE, FILE_APPEND);
        if (!file) return;
        file.printf("SAMPLE,%d,%u,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,,,,,%s\n",
                    section, (unsigned)timestamp, distance,
                    pitch, roll, zLeft, zRight, zRear, safetyStatus.c_str());
        file.close();
    }

    // Section summary row, written once a section boundary is crossed.
    void logSection(int section, float distanceEnd,
                    float pitchAvg, float rollAvg,
                    float zLeftAvg, float zRightAvg, float zRearAvg,
                    int samples,
                    float rangeLeft, float rangeRight, float rangeRear) {
        File file = LittleFS.open(DATA_FILE, FILE_APPEND);
        if (!file) return;
        file.printf("SECTION,%d,,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%.2f,%.2f,%.2f,,\n",
                    section, distanceEnd,
                    pitchAvg, rollAvg,
                    zLeftAvg, zRightAvg, zRearAvg,
                    samples,
                    rangeLeft, rangeRight, rangeRear);
        file.close();
    }

    // ------------------------------------------------------------------
    // Geometry / section configuration (persisted as JSON)
    // ------------------------------------------------------------------
    void loadConfig(float* frontTrackMm, float* rearArmMm, float* sectionMm) {
        if (!LittleFS.exists(CONFIG_FILE)) {
            *frontTrackMm = DEFAULT_FRONT_TRACK_MM;
            *rearArmMm    = DEFAULT_REAR_ARM_MM;
            *sectionMm    = DEFAULT_SECTION_MM;
            saveConfig(*frontTrackMm, *rearArmMm, *sectionMm);
            Serial.println("[+] Geometry config initialized with defaults.");
            return;
        }
        File f = LittleFS.open(CONFIG_FILE, FILE_READ);
        if (!f) {
            *frontTrackMm = DEFAULT_FRONT_TRACK_MM;
            *rearArmMm    = DEFAULT_REAR_ARM_MM;
            *sectionMm    = DEFAULT_SECTION_MM;
            Serial.println("[-] Could not open config file, using defaults.");
            return;
        }
        String body = f.readString();
        f.close();

        *frontTrackMm = jsonFloat(body, "front_track_mm", DEFAULT_FRONT_TRACK_MM);
        *rearArmMm    = jsonFloat(body, "rear_arm_mm",    DEFAULT_REAR_ARM_MM);
        *sectionMm    = jsonFloat(body, "section_mm",     DEFAULT_SECTION_MM);

        if (*sectionMm    < 10.0f) *sectionMm    = DEFAULT_SECTION_MM;
        if (*frontTrackMm <= 0.0f) *frontTrackMm = DEFAULT_FRONT_TRACK_MM;
        if (*rearArmMm    <= 0.0f) *rearArmMm    = DEFAULT_REAR_ARM_MM;

        Serial.printf("[+] Geometry loaded: track=%.0fmm rear=%.0fmm section=%.0fmm\n",
                      *frontTrackMm, *rearArmMm, *sectionMm);
    }

    bool saveConfig(float frontTrackMm, float rearArmMm, float sectionMm) {
        String json = "{";
        json += "\"front_track_mm\":"; json += String(frontTrackMm, 2); json += ",";
        json += "\"rear_arm_mm\":";    json += String(rearArmMm, 2);    json += ",";
        json += "\"section_mm\":";     json += String(sectionMm, 2);
        json += "}";

        File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
        if (!f) {
            Serial.println("[-] Could not write config file.");
            return false;
        }
        f.print(json);
        f.close();
        Serial.printf("[+] Geometry saved: track=%.0fmm rear=%.0fmm section=%.0fmm\n",
                      frontTrackMm, rearArmMm, sectionMm);
        return true;
    }
};

#endif
