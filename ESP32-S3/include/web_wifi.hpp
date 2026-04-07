#ifndef WEB_WIFI_HPP
#define WEB_WIFI_HPP

// web_wifi.hpp
#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h>
#include <WebServer.h>
#include <Wifi.h>

#include "web_utils.hpp"
#include "definiciones.hpp"
#include "config_params.hpp"
#include "config_prefs.hpp"
#include "RS485.hpp"
#include "rele.hpp"
#include "logBuf.hpp"

// Objeto global del servidor (útil si necesitas acceder a él desde main)
extern WebServer serverWiFi;

// Configuración inicial (registra las rutas)
void setupWebWiFi();

// Bucle principal (llamar en loop())
void webHandleClientWiFi();

#endif // WEB_WIFI_HPP