#pragma once
#include <Arduino.h>

// ============================================================================
// W5500 — Inicialización de Ethernet (SPI + IP estática o DHCP)
// Define pines y MAC. Llama a begin_ethernet() en setup().
// ============================================================================

// Pines SPI del módulo W5500 (ajusta a tu placa si difieren)
#ifndef ETH_SCK
  #define ETH_SCK   18
#endif
#ifndef ETH_MISO
  #define ETH_MISO  19
#endif
#ifndef ETH_MOSI
  #define ETH_MOSI  23
#endif
#ifndef ETH_CS_PIN
  #define ETH_CS_PIN 5
#endif

// MAC del dispositivo (única por equipo)
extern uint8_t g_mac[6];

// Arranque de Ethernet (SPI + W5500 + IP)
void begin_ethernet();
