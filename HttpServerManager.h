#ifndef HTTP_SERVER_MANAGER_H
#define HTTP_SERVER_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <math.h>
#include "Config.h"
#include "WebHtml.h"
#include "WebScript.h"
#include "StorageManager.h"

extern bool isMeasuring;
extern float globalDistance;
extern float globalPitch;
extern float globalRoll;
extern void resetDataSystem();
extern void triggerIMUCalibrate();

// 3-point geometry + live wheel heights (owned by FloorMeter.ino)
extern float g_frontTrackMm;
extern float g_rearArmMm;
extern float g_sectionMm;
extern float g_zLeftMm;
extern float g_zRightMm;
extern float g_zRearMm;

extern StorageManager storage;

class HttpServerManager {
private:
    WebServer server;

    void handleRoot() {
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");

        server.sendContent_P(HTML_PART_1);
        server.sendContent_P(HTML_PART_2);
        server.sendContent("");
    }

    void handleAction() {
        String cmd = server.arg("cmd");
        if (cmd == "start") {
            resetDataSystem(); 
            isMeasuring = true;
        }
        if (cmd == "stop") {
            isMeasuring = false;
        }
        if (cmd == "recal") {
            triggerIMUCalibrate(); 
        }
        
        String stateStr = isMeasuring ? "true" : "false";
        String responseJson = "{\"measuring\":" + stateStr + "}";
        server.send(200, "application/json", responseJson);
    }

    void handleGetData() {
        // Distance is reported in centimeters, whole number, never negative
        // (clamped so encoder backward drift can never show a minus value).
        float distCm = globalDistance * 100.0f;
        if (distCm < 0.0f) distCm = 0.0f;
        String dStr = String((long)(distCm + 0.5f));
        String pStr = String(globalPitch, 1);
        String rStr = String(globalRoll, 1);
        String mStr = isMeasuring ? "true" : "false";
        // Wheel heights: whole mm (signed) so sub-mm sensor jitter is not visible
        String zlStr = String((long)lroundf(g_zLeftMm));
        String zrStr = String((long)lroundf(g_zRightMm));
        String zrearStr = String((long)lroundf(g_zRearMm));

        String json = "{";
        json += "\"dist\":" + dStr + ",";
        json += "\"pitch\":" + pStr + ",";
        json += "\"roll\":" + rStr + ",";
        json += "\"z_left\":" + zlStr + ",";
        json += "\"z_right\":" + zrStr + ",";
        json += "\"z_rear\":" + zrearStr + ",";
        json += "\"measuring\":" + mStr;
        json += "}";
        server.send(200, "application/json", json);
    }

    String configJson() {
        String out = "{";
        out += "\"front_track_mm\":" + String(g_frontTrackMm, 2) + ",";
        out += "\"rear_arm_mm\":" + String(g_rearArmMm, 2) + ",";
        out += "\"section_mm\":" + String(g_sectionMm, 2);
        out += "}";
        return out;
    }

    void handleConfigGet() {
        server.send(200, "application/json", configJson());
    }

    void handleConfigPost() {
        float front = server.arg("front_track_mm").toFloat();
        float rear  = server.arg("rear_arm_mm").toFloat();
        float sec   = server.arg("section_mm").toFloat();
        if (front <= 0.0f) front = DEFAULT_FRONT_TRACK_MM;
        if (rear  <= 0.0f) rear  = DEFAULT_REAR_ARM_MM;
        if (sec   < 10.0f) sec   = DEFAULT_SECTION_MM;
        g_frontTrackMm = front;
        g_rearArmMm    = rear;
        g_sectionMm    = sec;
        if (storage.saveConfig(front, rear, sec)) {
            server.send(200, "application/json", configJson());
        } else {
            server.send(500, "text/plain", "Failed to save config");
        }
    }

    void handleDownload() {
        if (LittleFS.exists(DATA_FILE)) {
            File file = LittleFS.open(DATA_FILE, FILE_READ);
            server.streamFile(file, "text/csv");
            file.close();
        } else {
            server.send(404, "text/plain", "File empty.");
        }
    }

public:
    HttpServerManager() : server(80) {}

    void begin() {
        WiFi.softAP(AP_SSID, AP_PASS);
        
        server.on("/", std::bind(&HttpServerManager::handleRoot, this));
        server.on("/action", std::bind(&HttpServerManager::handleAction, this));
        server.on("/getdata", std::bind(&HttpServerManager::handleGetData, this));
        server.on("/download", std::bind(&HttpServerManager::handleDownload, this));
        server.on("/config", HTTP_GET, std::bind(&HttpServerManager::handleConfigGet, this));
        server.on("/config", HTTP_POST, std::bind(&HttpServerManager::handleConfigPost, this));

        server.begin();
        Serial.print("[+] Dashboard Ready at: ");
        Serial.println(WiFi.softAPIP());
    }

    void handleClient() {
        server.handleClient();
    }
};

#endif
