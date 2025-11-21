#include "rele.hpp"

static inline void releOn(uint8_t pin) { digitalWrite(pin, HIGH); }
static inline void releOff(uint8_t pin) { digitalWrite(pin, LOW); }

void rele_begin()
{
  pinMode(PIN_RELE, OUTPUT);
  releOff(PIN_RELE);
}

void rele_open()
{

  releOn(PIN_RELE);
  vTaskDelay(pdMS_TO_TICKS(1000));
  releOff(PIN_RELE);
}

void rele_close()
{

  releOff(PIN_RELE);
}