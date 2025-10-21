#include "rele.hpp"

static inline void releOn(uint8_t pin){ digitalWrite(pin, LOW); }
static inline void releOff(uint8_t pin){ digitalWrite(pin, HIGH); }

void rele_begin(){
  pinMode(PIN_RELE_ENT, OUTPUT);
  pinMode(PIN_RELE_SAL, OUTPUT);
  releOff(PIN_RELE_ENT);
  releOff(PIN_RELE_SAL);
}

void rele_open_entrada(){

  Serial.println("Activando rele entrada");
  releOn(PIN_RELE_ENT);
  vTaskDelay(pdMS_TO_TICKS(1000));
  releOff(PIN_RELE_ENT);
  Serial.println("Desactivando rele entrada");  
}

void rele_open_salida(){
  Serial.println("Activando rele salida");
  releOn(PIN_RELE_SAL);
  vTaskDelay(pdMS_TO_TICKS(1000));
  releOff(PIN_RELE_SAL);
  Serial.println("Desactivando rele salida");
}
