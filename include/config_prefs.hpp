#pragma once
#include <Arduino.h>
#include <IPAddress.h>

struct TornoConfig {
  String deviceId;   // nombrePlaca  -> DEVICE_ID
  String sentido;    // sentido
  uint32_t aperturasTotales; // aperturasTotales
  uint8_t modoRed;   // modoRed
  String ip, gw, mask, dns1, dns2; // como texto
  String urlBase;    // urlBase -> serverURL
};

bool cfgBegin();
TornoConfig cfgLoad();
bool cfgSave(const TornoConfig& c);

// aplica config a las variables globales de definiciones.cpp
void cfgApplyToGlobals(const TornoConfig& c);
void cfgEnsureFirmwareDefaults();

// helpers
bool cfgParseIP(const String& s, IPAddress& out);
bool cfgValidateIP(const String& s);


