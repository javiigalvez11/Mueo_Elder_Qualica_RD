#pragma once

#include <Arduino.h>   // String
#include <Ethernet.h>  // EthernetClient

// Atiende a los clientes HTTP (llamar desde tu task/loop)
void webHandleClient();

// ================= Rutas públicas =================
// Login / menú
void handleRoot(EthernetClient &client);
void handleSubmit(EthernetClient &client, const String &body);
void handleMenu(EthernetClient &client);
void handleLogout(EthernetClient &client);

// Reinicio
void handleReiniciarPage(EthernetClient &client);
void handleReiniciarDispositivo(EthernetClient &client);

// Firmware OTA
void handleFirmwarePage(EthernetClient &client);

// LittleFS (página + upload)
void handleFsPage(EthernetClient &client);

// Config (Preferences)
void handleConfigPage(EthernetClient &client);
void handleConfigSave(EthernetClient &client, const String &body);

// Stats / contadores
void handleStats(EthernetClient &client);
void handleResetAperturas(EthernetClient &client);

