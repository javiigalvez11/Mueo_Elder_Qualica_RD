#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>

#define MACHINE_ID 0x01  // ajusta aquí el ID de máquina (1..99) según tu configuración

#define PIN_RS485_RX      41
#define PIN_RS485_TX      42  
#define RS485_BAUD        19200

// ======= Config/log =======
static uint8_t txBuf[8];
static uint8_t rxBuf[18];


namespace RS485 {

struct StatusFrame {
  bool     valid = false;
  uint8_t  version = 0;
  uint8_t  machine = 0;
  uint8_t  fault = 0;
  uint8_t  gate = 0;
  uint8_t  alarm = 0;
  uint32_t leftCount = 0;
  uint32_t rightCount = 0;
  uint8_t  infrared = 0;
  uint8_t  cmdExec = 0;
  uint8_t  vcc = 0;
  uint32_t lastMs = 0;
};

// --- Setup / control ---
void begin();
void setDebug(bool on);

// --- Ciclo ---
void poll();

// --- Estado ---
StatusFrame getStatus();

// --- Comandos RS485 (ajusta MACHINE_ID en tu config) ---
void queryDeviceStatus(uint8_t m);  
void resetLeftCount   (uint8_t m);
void resetRightCount  (uint8_t m);
void resetDevice      (uint8_t m);

void leftOpen         (uint8_t m, uint8_t p);
void leftAlwaysOpen   (uint8_t m);

void rightOpen        (uint8_t m, uint8_t p);
void rightAlwaysOpen  (uint8_t m);

void openGateAlways   (uint8_t m);
void closeGate        (uint8_t m);

void forbiddenLeftPassage    (uint8_t m);
void forbiddenRightPassage   (uint8_t m);
void disablePassageRestriccion(uint8_t m);

// Función genérica (útil para comandos no mapeados específicamente)
// La añadimos inline o en cpp para flexibilidad en el json
void sendCustomCmd(uint8_t machine, uint8_t cmd, uint8_t d0, uint8_t d1);

// Parámetros
bool setParam(uint8_t m, uint8_t menu, uint8_t value);

// --- Helpers web para parámetros ---
bool writeParam(uint8_t id, uint8_t value);
bool readParam(uint8_t id, uint8_t& value);

} // namespace RS485
