#pragma once
#include <Arduino.h>

struct TornoParams {
    uint16_t machineId;      // 0
    uint8_t  openingMode;    // 1
    uint16_t waitTime;       // 2
    uint8_t  voiceLeft;      // 3
    uint8_t  voiceRight;     // 4
    uint8_t  voiceVol;       // 5
    uint8_t  masterSpeed;    // 6
    uint8_t  slaveSpeed;     // 7
    uint8_t  debugMode;      // 8
    uint8_t  decelRange;     // 9
    uint8_t  selfTestSpeed;  // 10
    uint8_t  passageMode;    // 11
    uint8_t  closeControl;   // 12
    uint8_t  singleMotor;    // 13
    uint8_t  language;       // 14
    uint8_t  irRebound;      // 15
    uint8_t  pinchSens;      // 16
    uint8_t  reverseEntry;   // 17
    uint8_t  turnstileType;  // 18
    uint8_t  emergencyDir;   // 19
    uint8_t  motorResist;    // 20
    uint8_t  intrusionVoice; // 21
    uint16_t irDelay;        // 22
    uint8_t  motorDir;       // 23
    uint8_t  clutchLock;     // 24
    uint8_t  hallType;       // 25
    uint8_t  signalFilter;   // 26
    uint8_t  cardInside;     // 27
    uint8_t  tailgateAlarm;  // 28
    uint8_t  limitDev;       // 29
    uint8_t  pinchFree;      // 30
    uint8_t  memoryFree;     // 31
    uint8_t  slipMaster;     // 32
    uint8_t  slipSlave;      // 33
    uint8_t  irLogicMode;    // 34
    uint8_t  lightMaster;    // 35
    uint8_t  lightSlave;     // 36
};

bool paramsBegin();
TornoParams paramsLoad();
bool paramsSave(const TornoParams& p);

// Aplica los parámetros cargados a tus variables uint individuales
void paramsApplyToGlobals(const TornoParams& p);
void paramsEnsureDefaults();