//web.hpp
#ifndef WEB_HPP
#define WEB_HPP
// web.hpp
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <Update.h>

// Server HTTP global
extern WebServer serverAP;

// Rutas (ya las tienes implementadas en web.cpp)
void rutasServidor(WebServer &server_ref);

// Handlers (opcional, ya están en tu web.cpp; sólo los declaro si quieres llamar alguno directo)
void mensajeError();
void handleRoot();
void handleSubmit();
void handleMenu();
void handleReiniciarPage();
void handleReiniciarDispositivo();
void handleFirmwarePage();
void handleFirmwareUpload();
void handleFirmwareUploadComplete();
void handleLogout();

#endif // WEB_HPP