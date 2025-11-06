#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>

#define PIN_RELE 14

void rele_begin();
void rele_open();         // abre ambos relés
void rele_close();        // cierra ambos relés