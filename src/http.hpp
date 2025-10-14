#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// === Endpoints de alto nivel ===
void begin_wifi();  // inicializa WiFi STA (usa config.hpp)
void getInicio();       // POST /status (inicio)
void getEstado();       // POST /status (periódico)
void postTicket();      // POST /validateQR
void postPaso();        // POST /validatePass
void reportFailure();   // POST /reportFailure
void actualiza();       // OTA (usa httpUpdate)


// Arranca el webserver y guarda el handle de qToIO para mandar comandos a IO
void http_begin(QueueHandle_t qToIO);

// Debe llamarse periódicamente (taskNet) para atender peticiones
void http_poll();