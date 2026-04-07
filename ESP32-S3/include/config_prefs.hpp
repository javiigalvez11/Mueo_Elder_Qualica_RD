#pragma once
#include <Arduino.h>
#include <IPAddress.h>

struct TornoConfig {
  String deviceId;   // nombrePlaca  -> DEVICE_ID
  uint8_t mac[6];      // MAC en binario (6 bytes)
  String wifiSSID;   // SSID WiFi
  String wifiPass;   // Contraseña WiFi
  String urlBase;    // urlBase -> serverURL

  String urlActualiza; // urlActualiza

  uint8_t modoPasillo;
  uint8_t modoApertura;
  uint8_t sentidoApertura;
  uint32_t entradasTotales; 
  uint32_t salidasTotales;

  uint8_t conexionRed;
  uint8_t modoRed;   // modoRed
  String ip, gw, mask, dns1, dns2; // como texto
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


