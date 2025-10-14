#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>


void rele_begin();
void rele_open(uint32_t ms = 1500); // pulso de apertura
