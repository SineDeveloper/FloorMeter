#ifndef WEB_HTML_H
#define WEB_HTML_H

#include <Arduino.h>

const char HTML_PART_1[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Super Flat Floor Monitor</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; text-align: center; background: #f8f9fa; margin: 0; padding: 10px; color: #333; }
        .container { max-width: 800px; margin: auto; background: white; padding: 15px; border-radius: 6px; box-shadow: 0 1px 3px rgba(0,0,0,0.05); border: 1px solid #e1e4e6; }
        h2 { font-size: 16px; margin: 5px 0 15px 0; color: #222; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; }
        
        .btn { display: inline-block; padding: 6px 12px; font-size: 13px; color: #333; background: #fff; text-decoration: none; border-radius: 4px; margin: 2px; border: 1px solid #ccc; cursor: pointer; font-weight: 500; }
        .btn:hover:not(:disabled) { background: #f1f2f3; }
        .btn:disabled { opacity: 0.35; cursor: not-allowed; background: #eee !important; color: #999 !important; border-color: #ddd !important; }
        
        .btn-start { background: #e6f4ea; color: #137333; border-color: #ceead6; }
        .btn-start:hover:not(:disabled) { background: #d2e9d8; }
        .btn-stop { background: #fce8e6; color: #c5221f; border-color: #fad2cf; }
        .btn-stop:hover:not(:disabled) { background: #f7c5c0; }
        
        .status-box { font-size: 13px; margin: 10px 0; font-weight: 500; padding: 6px; border-radius: 4px; border: 1px solid #eee; background: #fafafa; color: #666; }
        .status-active { background: #e6f4ea; color: #137333; border-color: #ceead6; }
        
        .chart-wrapper { background: #1e1e1e; padding: 10px; border-radius: 4px; margin: 10px auto; }
        
        .data-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-bottom: 12px; }
        .data-card { background: #f1f3f4; padding: 6px; border-radius: 4px; font-size: 12px; color: #5f6368; border: 1px solid #e8eaed; } 
        .data-val { font-size: 16px; font-weight: 600; color: #202124; margin-top: 2px; }
        .data-val-z { font-size: 14px; }

        .config-panel { margin: 10px 0; padding: 10px; border: 1px solid #e1e4e6; border-radius: 4px; background: #fafafa; }
        .config-title { font-size: 12px; font-weight: 600; color: #222; text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 8px; }
        .config-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; align-items: end; }
        .config-field { font-size: 11px; color: #5f6368; text-align: left; }
        .config-field input { width: 100%; box-sizing: border-box; margin-top: 2px; padding: 4px 6px; border: 1px solid #ccc; border-radius: 4px; font-size: 13px; }
        .config-msg { font-size: 11px; margin-top: 6px; color: #5f6368; }
        .config-msg-ok { color: #137333; } .config-msg-err { color: #c5221f; }
        
        .safety-indicator { margin-top: 15px; padding: 8px; border-radius: 4px; font-size: 13px; font-weight: 500; border: 1px solid #ccc; text-transform: uppercase; }
        .safety-idle { background: #f1f3f4; color: #5f6368; border-color: #dadce0; }
        .safety-safe { background: #e6f4ea; color: #137333; border-color: #ceead6; }
        .safety-warn { background: #fef7e0; color: #b06000; border-color: #feebc8; }
        .safety-danger { background: #fce8e6; color: #c5221f; border-color: #fad2cf; }
        
        .legend { display: flex; justify-content: center; gap: 15px; color: #aaa; font-size: 11px; margin-bottom: 5px; flex-wrap: wrap; }
        .legend-p { color: #f28b82; font-weight: 500; } .legend-r { color: #8ab4f8; font-weight: 500; }
        .legend-lim { color: #fdd663; font-style: italic; }
    </style>
</head>
<body>
    <div class='container'>
        <h2>Super Flat Floor & Forklift Safety Monitor</h2>

        <div class='data-grid'>
            <div class='data-card'>DISTANCE (m)<div id='val-dist' class='data-val'>0.000</div></div>
            <div class='data-card'>PITCH (deg)<div id='val-pitch' class='data-val'>0.0</div></div>
            <div class='data-card'>ROLL (deg)<div id='val-roll' class='data-val'>0.0</div></div>
            <div class='data-card'>WHEEL Z LEFT (mm)<div id='val-zl' class='data-val data-val-z'>0.00</div></div>
            <div class='data-card'>WHEEL Z RIGHT (mm)<div id='val-zr' class='data-val data-val-z'>0.00</div></div>
            <div class='data-card'>REAR Z (mm)<div id='val-zrear' class='data-val data-val-z'>0.00</div></div>
        </div>

        <div class='config-panel'>
            <div class='config-title'>3-Point Geometry (mm)</div>
            <div class='config-grid'>
                <div class='config-field'>Front axle track<input id='cfg-front' type='number' step='0.1' value='800'></div>
                <div class='config-field'>Rear arm length (pivot to axle)<input id='cfg-rear' type='number' step='0.1' value='600'></div>
                <div class='config-field'>Section length (CSV grouping)<input id='cfg-section' type='number' step='10' value='300'></div>
            </div>
            <button onclick='saveConfig()' class='btn' style='margin-top:8px;'>Save Geometry</button>
            <div id='config-msg' class='config-msg'></div>
        </div>

        <button id='btn-start-id' onclick='sendCmd("start")' class='btn btn-start'>Start</button>
        <button id='btn-stop-id' onclick='sendCmd("stop")' class='btn btn-stop' disabled>Stop</button>
        <button id='btn-recal-id' onclick='recalibrateCmd()' class='btn'>Re-Calibrate</button>
        <a href='/download' class='btn'>Download CSV</a>

        <div id='status-msg' class='status-box'>READY / AWAITING COMMAND</div>

        <div class='chart-wrapper'>
            <div class='legend'>
                <span class='legend-p'>- Pitch (Longitudinal)</span>
                <span class='legend-r'>- Roll (Transverse)</span>
                <span class='legend-lim'>--- Safety Limit (Pitch 1.0° / Roll 0.5°)</span>
            </div>
            <canvas id='offlineChart' width='700' height='260' style='width:100%; height:260px;'></canvas>
        </div>

        <div id='safety-panel' class='safety-indicator safety-idle'>SYSTEM IDLE: AWAITING DATA RECORDING</div>
    </div>
)=====";

#endif
