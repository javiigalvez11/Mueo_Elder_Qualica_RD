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
#define ETH_SCLK 13  // Antes 18
#endif
#ifndef ETH_MISO
#define ETH_MISO 12  // Antes 19
#endif
#ifndef ETH_MOSI
#define ETH_MOSI 11  // Antes 23
#endif
#ifndef ETH_CS_PIN
#define ETH_CS_PIN 14 // Antes 5
#endif
#ifndef ETH_INT_PIN
#define ETH_INT_PIN 10 // Antes 10
#endif
#ifndef ETH_RST_PIN
#define ETH_RST_PIN 9 // Antes 25
#endif

// Arranque de Ethernet (SPI + W5500 + IP)
namespace W5500
{
  void begin();
  bool reintentarDHCP();
}
#endif // W5500_HPP