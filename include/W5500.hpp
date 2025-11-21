#ifndef W5500_HPP
#define W5500_HPP

#pragma once
#include <Arduino.h>

// ============================================================================
// W5500 — Inicialización de Ethernet (SPI + IP estática o DHCP)
// Define pines y MAC. Llama a begin_ethernet() en setup().
// ============================================================================

// Pines SPI del módulo W5500 (ajusta a tu placa si difieren)
#ifndef ETH_SCLK
  #define ETH_SCLK   18
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
#ifndef ETH_RST_PIN
  #define ETH_RST_PIN -1 // sin reset por defecto
#endif

// Arranque de Ethernet (SPI + W5500 + IP)
void begin_ethernet();

#endif // W5500_HPP