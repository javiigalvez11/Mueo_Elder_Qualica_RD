#include "RS485.hpp"
#include "config.hpp"


uint8_t startPos = 0, version_number = 0, machineNumber_ret = 0;
uint8_t faultEvent = 0, gateStatus = 0, alarmEvent = 0;
uint32_t leftCount = 0, rightCount = 0;
uint8_t infraredStatus = 0, commandExecStatus = 0, powerSupplyVolt = 0, undef1 = 0, undef2 = 0, checkSum = 0;

namespace RS485
{

  // ======= Config/log =======
  static HardwareSerial *S = nullptr;

  // ======= Utils =======
  static inline uint8_t checksum7(const uint8_t *buf, int len)
  {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
      sum += buf[i];
    return (uint8_t)~sum;
  }
  static inline bool verifyChecksum(const uint8_t *buf, int len)
  {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
      sum += buf[i];
    return (uint8_t)(sum + 1) == 0;
  }

  // Construye CMD: 7E 00 <machine> <cmd> <d0> <d1> <d2> <chk>
  static void buildCmd(uint8_t machine, uint8_t cmd, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t out[8])
  {
    out[0] = 0x7E;
    out[1] = 0x00;
    out[2] = machine;
    out[3] = cmd;
    out[4] = d0;
    out[5] = d1;
    out[6] = d2;
    out[7] = checksum7(out, 7);
    if (debugSerie)
    {
      Serial.print(F("[RS485] CMD: "));
      for (int i = 0; i < 8; i++)
        Serial.printf("0x%02X ", out[i]);
      Serial.println();
    }
  }

  // Envía el contenido del buffer txBuf (sin control DE/RE)
  static void sendTx()
  {
    if (!S)
      return;
    if (!verifyChecksum(txBuf, 8))
    {
      if (debugSerie)
        Serial.println(F("[RS485] !! checksum TX inválido, no envío"));
      return;
    }
    while (S->available())
      (void)S->read(); // limpia RX antes de esperar respuesta
    S->write(txBuf, 8);
    S->flush();

    if (debugSerie)
    {
      Serial.print(F("[RS485] TX: "));
      for (int i = 0; i < 8; i++)
        Serial.printf("0x%02X ", txBuf[i]);
      Serial.println();
    }
  }

  // ======= API de comandos =======
  void queryDeviceStatus(uint8_t m)
  {
    buildCmd(m, 0x10, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }

  void resetLeftCount(uint8_t m)
  {
    buildCmd(m, 0x20, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }
  void resetRightCount(uint8_t m)
  {
    buildCmd(m, 0x21, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }
  void resetDevice(uint8_t m)
  {
    buildCmd(m, 0x35, 0x60, 0x00, 0x00, txBuf);
    sendTx();
  }

  void leftOpen(uint8_t m,int pasos)
  {
    buildCmd(m, 0x80, pasos, 0x00, 0x00, txBuf);
    sendTx();
  }
  void leftAlwaysOpen(uint8_t m)
  {
    buildCmd(m, 0x81, 0x01, 0x00, 0x00, txBuf);
    sendTx();
  }

  void rightOpen(uint8_t m, int pasos)
  {
    buildCmd(m, 0x82, 0x01, 0x00, 0x00, txBuf);
    sendTx();
  }
  void rightAlwaysOpen(uint8_t m)
  {
    buildCmd(m, 0x83, 0x01, 0x00, 0x00, txBuf);
    sendTx();
  }

  void openGateAlways(uint8_t m)
  {
    buildCmd(m, 0x83, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }
  void closeGate(uint8_t m)
  {
    buildCmd(m, 0x84, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }

  void forbiddenLeftPassage(uint8_t m)
  {
    buildCmd(m, 0x88, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }
  void forbiddenRightPassage(uint8_t m)
  {
    buildCmd(m, 0x89, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }
  void disablePassageRestriccion(uint8_t m)
  {
    buildCmd(m, 0x8F, 0x00, 0x00, 0x00, txBuf);
    sendTx();
  }

  void setParam(uint8_t m, uint8_t menu, uint8_t value)
  {
    buildCmd(m, 0x96, menu, value, 0x00, txBuf);
    sendTx();
  }

  // Helpers web
  bool writeParam(uint8_t id, uint8_t value)
  {
    setParam(MACHINE_ID, id, value);
    return true;
  }
  bool readParam(uint8_t /*id*/, uint8_t & /*value*/)
  {
    return false; // no hay opcode de lectura individual
  }

  // ======= Parseo de frame 0x7F…(18B) =======
  static void parseStatusFrame()
  {
    startPos = rxBuf[0];
    version_number = rxBuf[1];
    machineNumber_ret = rxBuf[2];
    faultEvent = rxBuf[3];
    gateStatus = rxBuf[4];
    alarmEvent = rxBuf[5];

    leftCount = ((uint32_t)rxBuf[6] << 16) | ((uint32_t)rxBuf[7] << 8) | (uint32_t)rxBuf[8];
    rightCount = ((uint32_t)rxBuf[9] << 16) | ((uint32_t)rxBuf[10] << 8) | (uint32_t)rxBuf[11];

    infraredStatus = rxBuf[12];
    commandExecStatus = rxBuf[13];
    powerSupplyVolt = rxBuf[14];
    undef1 = rxBuf[15];
    undef2 = rxBuf[16];
    checkSum = rxBuf[17];

    if (debugSerie)
    {
      Serial.println(F("===== RX STATUS (18B) ====="));
      Serial.printf("Start:0x%02X Ver:0x%02X Machine:%u\n", startPos, version_number, machineNumber_ret);
      Serial.printf("Fault:0x%02X Gate:0x%02X Alarm:0x%02X\n", faultEvent, gateStatus, alarmEvent);
      Serial.printf("Left:%lu Right:%lu Infrared:0x%02X CmdExec:0x%02X Vcc:%u\n",
                    (unsigned long)leftCount, (unsigned long)rightCount, infraredStatus, commandExecStatus, powerSupplyVolt);
      Serial.println(F("============================"));
    }
  }

  // ======= Lector no-bloqueante =======
  void poll()
  {
    if (!S)
      return;

    constexpr uint32_t INTERBYTE_TIMEOUT_MS = 80;
    constexpr uint32_t FRAME_TIMEOUT_MS = 300;

    static bool inFrame = false;
    static uint8_t idx = 0;
    static uint32_t tLastByte = 0;
    static uint32_t tStart = 0;

    while (S->available())
    {
      uint8_t b = (uint8_t)S->read();
      uint32_t now = millis();

      if (inFrame && (uint32_t)(now - tLastByte) > INTERBYTE_TIMEOUT_MS)
      {
        if (debugSerie)
        {
          Serial.printf("[RS485] inter-byte timeout (%lums). Descarto parcial: %u bytes\n",
                        (unsigned long)(now - tLastByte), idx);
        }
        inFrame = false;
        idx = 0;
      }

      if (!inFrame)
      {
        if (b == 0x7F)
        {
          inFrame = true;
          idx = 0;
          tStart = now;
          tLastByte = now;
          rxBuf[idx++] = b;

          if (debugSerie)
            Serial.println(F("[RS485] <-- start 0x7F"));
        }
        continue;
      }

      rxBuf[idx++] = b;
      tLastByte = now;

      if ((uint32_t)(now - tStart) > FRAME_TIMEOUT_MS)
      {
        if (debugSerie)
        {
          Serial.printf("[RS485] frame timeout total (%lums) con %u bytes. Descarto.\n",
                        (unsigned long)(now - tStart), idx);
        }
        inFrame = false;
        idx = 0;
        continue;
      }

      if (idx >= sizeof(rxBuf))
      {
        
        inFrame = false;
        uint8_t ok = verifyChecksum(rxBuf, (int)sizeof(rxBuf));

        if (!ok)
        {
          if (debugSerie)
          {
            Serial.println(F("[RS485] RX checksum inválido. Dump:"));
            for (int i = 0; i < 18; i++)
              Serial.printf("0x%02X ", rxBuf[i]);
            Serial.println();
          }
          idx = 0;
          continue;
        }

        if (debugSerie)
        {
          Serial.print(F("[RS485] RX OK (18B, "));
          Serial.print((unsigned long)(now - tStart));
          Serial.println(F(" ms):"));
          for (int i = 0; i < 18; i++)
            Serial.printf("0x%02X ", rxBuf[i]);
          Serial.println();
        }

        parseStatusFrame();
        idx = 0;
      }
    }
  }

  // ======= Setup / Estado =======
  void begin(HardwareSerial &serial, uint32_t baud, int rxPin, int txPin)
  {
    S = &serial;
    S->begin(baud, SERIAL_8N1, rxPin, txPin);
    if (debugSerie)
    {
      Serial.println(F("[RS485] begin() OK"));
      Serial.printf("  UART: RX=%d TX=%d\n", rxPin, txPin);
    }
  }

  StatusFrame getStatus()
  {
    StatusFrame st;
    st.version = version_number;
    st.machine = machineNumber_ret;
    st.fault = faultEvent;
    st.gate = gateStatus;
    st.alarm = alarmEvent;
    st.leftCount = leftCount;
    st.rightCount = rightCount;
    st.infrared = infraredStatus;
    st.cmdExec = commandExecStatus;
    st.vcc = powerSupplyVolt;
    st.valid = (startPos == 0x7F);
    st.lastMs = millis();
    return st;
  }

  void setDebug(bool on) { debugSerie = on; }

} // namespace RS485
