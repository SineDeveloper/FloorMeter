#ifndef CONFIG_H
#define CONFIG_H

const char* AP_SSID = "SuperFlat_FloorMeter";
const char* AP_PASS = "123456789"; 

const char* DATA_FILE   = "/floor_data.csv";
const char* CONFIG_FILE = "/config.json";

// Actual wheel diameter (mm)
const float WHEEL_DIAMETER_MM = 100.0;

// ---- Geometry defaults (mm) -------------------------------------------
// Overridable at runtime from the web dashboard; persisted to CONFIG_FILE.
// FRONT_TRACK_MM : distance between the two front wheels (axle width).
// REAR_ARM_MM    : longitudinal distance from the front axle to the rear wheel.
// SECTION_MM     : travel distance that forms one measured section (default 300 mm = 30 cm).
const float DEFAULT_FRONT_TRACK_MM = 800.0;
const float DEFAULT_REAR_ARM_MM    = 600.0;
const float DEFAULT_SECTION_MM     = 300.0;

// Sign conventions for converting angles -> wheel heights.
// Set any of these to -1.0 to flip a wheel's "up is positive" direction if
// your physical mounting differs from the default assumption.
const float SIGN_ROLL_LEFT  = -1.0f;  // Z_Left  = SIGN_ROLL_LEFT * (FRONT_TRACK/2) * tan(roll)
const float SIGN_ROLL_RIGHT = +1.0f;  // Z_Right = SIGN_ROLL_RIGHT * (FRONT_TRACK/2) * tan(roll)
const float SIGN_PITCH_REAR = -1.0f;  // Z_Rear  = SIGN_PITCH_REAR * REAR_ARM * tan(pitch)
#endif
