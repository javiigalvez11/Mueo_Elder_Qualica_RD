#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>

#define PIN_RELE_ENT 26
#define PIN_RELE_SAL 27

void rele_begin();
void rele_open_entrada(); // pulso de apertura
void rele_open_salida();  // pulso de apertura