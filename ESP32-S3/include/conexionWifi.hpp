#pragma once
#include <Arduino.h>
#include <WiFi.h>

namespace WIFI{
    
    bool begin();
    void startFallbackAP();
    bool isConnected();
    void reconnect();
    
} 