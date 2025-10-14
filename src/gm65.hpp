#pragma once
#include <Arduino.h>



bool gm65_begin(HardwareSerial &serial, int pin_rx, int pin_tx, uint32_t baud); // inicializa GM65 (UART, pines, baudrate)
bool gm65_readLine(String &out); // devuelve línea terminada (sin \r\n)
