#pragma once
#include <Arduino.h>


#define PIN_GM65_RX  26
#define PIN_GM65_TX  27

#define GM65_BAUD    9600

bool gm65_begin(HardwareSerial &serial, int pin_rx, int pin_tx, uint32_t baud); // inicializa GM65 (UART, pines, baudrate)
bool gm65_readLine(String &out); // devuelve línea terminada (sin \r\n)
