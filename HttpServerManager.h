#ifndef HTTP_SERVER_MANAGER_H
#define HTTP_SERVER_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "Config.h"
#include "WebHtml.h"
#include "WebScript.h"

extern bool isMeasuring;
extern float globalDistance;
extern float globalPitch;
extern float globalRoll;
extern void resetDataSystem();
extern void triggerIMUCalibrate();

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
        String dStr = String(globalDistance, 3);
        String pStr = String(globalPitch, 1);
        String rStr = String(globalRoll, 1);
        String mStr = isMeasuring ? "true" : "false";

        String json = "{";
        json += "\"dist\":" + dStr + ",";
        json += "\"pitch\":" + pStr + ",";
        json += "\"roll\":" + rStr + ",";
        json += "\"measuring\":" + mStr;
        json += "}";
        server.send(200, "application/json", json);
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

        server.begin();
        Serial.print("[+] Dashboard Ready at: ");
        Serial.println(WiFi.softAPIP());
    }

    void handleClient() {
        server.handleClient();
    }
};

#endif
