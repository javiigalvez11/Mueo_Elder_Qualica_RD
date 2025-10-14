#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.hpp"

// === RS485 ===
static uint8_t txBuf[8];
static uint8_t rxBuf[18];

namespace RS485
{

  struct StatusFrame
  {
    bool valid = false;
    uint8_t version = 0;
    uint8_t machine = 0;
    uint8_t fault = 0;
    uint8_t gate = 0;
    uint8_t alarm = 0;
    uint32_t leftCount = 0;
    uint32_t rightCount = 0;
    uint8_t infrared = 0;
    uint8_t cmdExec = 0;
    uint8_t vcc = 0;
    uint32_t lastMs = 0;
  };

  // --- Setup / control ---
  void begin(HardwareSerial &serial, uint32_t baud, int rxPin, int txPin);
  void setDebug(bool on);

  // --- Ciclo ---
  void poll();

  // --- Estado ---
  StatusFrame getStatus();

  // --- Comandos RS485 (ajusta MACHINE_ID en tu config) ---
  void queryDeviceStatus(uint8_t m);

  void resetLeftCount(uint8_t m);
  void resetRightCount(uint8_t m);
  void resetDevice(uint8_t m);

  void leftOpen(uint8_t m);
  void leftAlwaysOpen(uint8_t m);

  void rightOpen(uint8_t m);
  void rightAlwaysOpen(uint8_t m);

  void openGateAlways(uint8_t m);
  void closeGate(uint8_t m);

  void forbiddenLeftPassage(uint8_t m);
  void forbiddenRightPassage(uint8_t m);
  void disablePassageRestriccion(uint8_t m);

  // --- Helpers web para parámetros ---
  bool writeParam(uint8_t id, uint8_t value);
  bool readParam(uint8_t id, uint8_t &value);

} // namespace RS485
