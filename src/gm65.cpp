#include <HardwareSerial.h>
#include "config.hpp"
#include "gm65.hpp"


// gm65.cpp
static HardwareSerial* g_uart = nullptr;

bool gm65_begin(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud) {
  serial.begin(baud, SERIAL_8N1, rxPin, txPin);
  g_uart = &serial;
  if (debugSerie) {
    Serial.println(F("[GM65] begin() OK"));
    Serial.printf("  UART: RX=%d TX=%d @%lu\n", rxPin, txPin, (unsigned long)baud);
  }
  return true;
}

bool gm65_readLine(String &out) {
  if (!g_uart) return false;
  static String acc;

  while (g_uart->available()) {
    char c = (char)g_uart->read();

    if (c == '\r' || c == '\n') {
      if (acc.length()) {
        acc.trim();                     // limpia espacios/CRLF
        if (acc.startsWith("ME")) {     // **filtro**: debe empezar por "ME"
          out = acc;
          acc = "";
          return true;                  // línea válida
        } else {
          if (debugSerie) {             // log opcional
            Serial.print("[GM65] descartado: ");
            Serial.println(acc);
          }
          acc = "";                     // descarta y sigue leyendo
        }
      }
    } else {
      acc += c;
      if (acc.length() > 256) acc.remove(0);  // evita desbordar buffer
    }
  }
  return false;                          // no hay línea válida aún
}
