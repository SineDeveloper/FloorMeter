#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <LittleFS.h>
#include "Config.h"

class StorageManager {
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
        file.println("Timestamp(ms),Distance(m),Pitch(deg),Roll(deg),Safety_Status");
        file.close();
        Serial.println("[+] Data file initialized with safety headers.");
    }

    void logData(uint32_t timestamp, float distance, float pitch, float roll, String safetyStatus) {
        File file = LittleFS.open(DATA_FILE, FILE_APPEND);
        if (file) {
            file.printf("%u,%.3f,%.1f,%.1f,%s\n", timestamp, distance, pitch, roll, safetyStatus.c_str());
            file.close();
        }
    }

    void clearData() {
        if (LittleFS.exists(DATA_FILE)) {
            LittleFS.remove(DATA_FILE);
            initLogFile();
            Serial.println("[+] Data file cleared.");
        }
    }
};

#endif
