#pragma once

#include <Arduino.h>
#include <WebServer.h>


// ========================= Servidor AP =========================

// Instancia del servidor WebServer en modo AP
extern WebServer serverAP;

// ========================= Registro de rutas =========================

// Registra todas las rutas del portal AP
void rutasServidor(WebServer &server_ref);
