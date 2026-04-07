#include "rele.hpp"

namespace rele
{
  // Funciones internas
  static inline void releOn(uint8_t pin) { digitalWrite(pin, HIGH); }
  static inline void releOff(uint8_t pin) { digitalWrite(pin, LOW); }

  void begin()
  {
    if(debugSerie)
      Serial.println("[RELE] Inicializando relés");
    pinMode(PIN_RELE_D, OUTPUT);
    releOff(PIN_RELE_D);

    pinMode(PIN_RELE_I, OUTPUT);
    releOff(PIN_RELE_I);
  }

  void openEntry()
  {
    if(debugSerie)
      Serial.println("[RELE] Abriendo puerta de entrada");
    releOn(PIN_RELE_D);
    vTaskDelay(pdMS_TO_TICKS(1000));
    releOff(PIN_RELE_D);
  }

  void openExit()
  {
    if(debugSerie)
      Serial.println("[RELE] Abriendo puerta de salida");
    releOn(PIN_RELE_I);
    vTaskDelay(pdMS_TO_TICKS(1000));
    releOff(PIN_RELE_I);
  }

  void close()
  {
    if(debugSerie)
      Serial.println("[RELE] Cerrando puertas");
    releOff(PIN_RELE_D);
    releOff(PIN_RELE_I);
  }
}