#include "RS485.hpp"
#include "definiciones.hpp"
#include "logBuf.hpp"

// Definición de variable global para el puntero serial
static HardwareSerial *r_uart = nullptr;

namespace RS485
{

  // ======= Utils =======

  // Corrección de seguridad: Aseguramos que la suma se comporte como uint8_t antes de invertir
  static inline uint8_t checksum7(const uint8_t *buf, int len)
  {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
    {
      sum += buf[i];
    }
    return (uint8_t)~sum;
  }

  // Verificación: Suma + Checksum + 1 debe ser 0 (Logic: Sum + ~Sum = 0xFF, +1 = 0x00)
  static inline bool verifyChecksum(const uint8_t *buf, int len)
  {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
    {
      sum += buf[i];
    }
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
      Serial.print(F("[RS485] BUILD CMD: "));
      for (int i = 0; i < 8; i++)
        Serial.printf("0x%02X ", out[i]);
      Serial.println();
    }

    logbuf_pushf("[RS485] BUILD CMD: 0x%02X", cmd);
  }

  // Envía el contenido del buffer txBuf
  static void sendTx()
  {
    if (!r_uart)
    {
      if (debugSerie)
        Serial.println(F("[RS485] ERROR: UART no inicializado"));
      return;
    }

    // --- CORRECCIÓN 3: Comentamos la limpieza agresiva ---
    // A veces esto borra respuestas entrantes si el loop es rápido.
    // while (r_uart->available()) (void)r_uart->read();

    // Escribir al bus
    r_uart->write(txBuf, 8);

    // Esperar a que el hardware termine de transmitir bits
    r_uart->flush();

    // --- CORRECCIÓN 1: EL DELAY FALTANTE ---
    // El código original tenía delay(200). RS485 necesita tiempo de turnaround.
    // 50ms suele ser suficiente, pero pon 200 si sigue fallando.
    delay(50);
  }

// ======= API de comandos =======
// Nota: MACHINE_ID debe estar definido en RS485.hpp o aquí
#ifndef MACHINE_ID
#define MACHINE_ID 0x01
#endif

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

  void leftOpen(uint8_t m, uint8_t p)
  {
    buildCmd(m, 0x80, p, 0x00, 0x00, txBuf);
    sendTx();
  }
  void leftAlwaysOpen(uint8_t m)
  {
    buildCmd(m, 0x81, 0x01, 0x00, 0x00, txBuf);
    sendTx();
  }

  void rightOpen(uint8_t m, uint8_t p)
  {
    buildCmd(m, 0x82, p, 0x00, 0x00, txBuf);
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

  // Implementación de CustomCmd
  void sendCustomCmd(uint8_t machine, uint8_t cmd, uint8_t d0, uint8_t d1)
  {
    buildCmd(machine, cmd, d0, d1, 0x00, txBuf);
    sendTx();
  }

  bool setParam(uint8_t m, uint8_t menu, uint8_t value)
  {
    buildCmd(m, 0x96, menu, value, 0x00, txBuf);
    sendTx();
    return true;
  }

  // Helpers web
  bool writeParam(uint8_t id, uint8_t value)
  {
    setParam(MACHINE_ID, id, value);
    return true;
  }
  bool readParam(uint8_t /*id*/, uint8_t & /*value*/)
  {
    return false;
  }

  // ======= Parseo de frame 0x7F…(18B) =======
  static void parseStatusFrame()
  {
    // Asegúrate de que estas variables globales existen y son accesibles
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

    // Solo imprimimos cada cierto tiempo para no saturar si el poll es muy rápido
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000)
    {
      if (debugSerie)
      {
        Serial.println(F("===== RX STATUS (RS485 Heartbeat) ====="));
        Serial.printf(" StartPos: 0x%02X\n", startPos);
        Serial.printf(" Version: 0x%02X\n", version_number);
        Serial.printf(" MachineID: 0x%02X\n", machineNumber_ret);
        Serial.printf(" FaultEvent: 0x%02X\n", faultEvent);
        Serial.printf(" GateStatus: 0x%02X\n", gateStatus);
        Serial.printf(" AlarmEvent: 0x%02X\n", alarmEvent);
        Serial.printf(" LeftCount: %lu\n", leftCount);
        Serial.printf(" RightCount: %lu\n", rightCount);
        Serial.printf(" InfraredStatus: 0x%02X\n", infraredStatus);
        Serial.printf(" CommandExecStatus: 0x%02X\n", commandExecStatus);
        Serial.printf(" PowerSupplyVolt: %u\n", powerSupplyVolt);
        Serial.printf(" Checksum: 0x%02X\n", checkSum);
        Serial.println("========================================");
        lastPrint = millis();
      }
    }
  }

  // ======= Lector no-bloqueante =======
  void poll()
  {
    if (!r_uart)
      return;

    constexpr uint32_t INTERBYTE_TIMEOUT_MS = 50; // Reducido un poco para ser más ágil
    constexpr uint32_t FRAME_TIMEOUT_MS = 300;

    static bool inFrame = false;
    static uint8_t idx = 0;
    static uint32_t tLastByte = 0;
    static uint32_t tStart = 0;

    while (r_uart->available())
    {
      uint8_t b = (uint8_t)r_uart->read();
      uint32_t now = millis();

      if (inFrame && (uint32_t)(now - tLastByte) > INTERBYTE_TIMEOUT_MS)
      {
        inFrame = false;
        idx = 0; // Timeout entre bytes, descartar
      }

      if (!inFrame)
      {
        if (b == 0x7F)
        { // Start Byte de RESPUESTA (TX usa 7E, RX usa 7F)
          inFrame = true;
          idx = 0;
          tStart = now;
          tLastByte = now;
          rxBuf[idx++] = b;
        }
        continue;
      }

      rxBuf[idx++] = b;
      tLastByte = now;

      if ((uint32_t)(now - tStart) > FRAME_TIMEOUT_MS)
      {
        inFrame = false;
        idx = 0;
        continue;
      }

      if (idx >= sizeof(rxBuf))
      { // Buffer lleno (18 bytes)
        inFrame = false;
        // IMPORTANTE: verifyChecksum aquí usa la misma lógica que sendTx
        // Si esto devuelve true, significa que el cálculo del checksum es correcto para ambos lados.
        uint8_t ok = verifyChecksum(rxBuf, (int)sizeof(rxBuf));

        if (ok)
        {
          parseStatusFrame();
        }
        else
        {
          if (debugSerie)
            Serial.println("[RS485] Checksum ERROR en RX");
        }
        idx = 0;
      }
    }
  }

  // ======= Setup / Estado =======
  void begin()
  {
    // Aseguramos que usamos los pines definidos
    // Nota: Serial2 en ESP32 es hardware.
    Serial2.begin(RS485_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    r_uart = &Serial2;

    if (debugSerie)
    {
      Serial.println(F("[RS485] begin() OK"));
      Serial.printf(" UART Pins: RX=%d TX=%d Baud=%d\n", PIN_RS485_RX, PIN_RS485_TX, RS485_BAUD);
    }

    logbuf_pushf("[RS485] begin() OK");
    logbuf_pushf(" UART Pins: RX=%d TX=%d Baud=%d", PIN_RS485_RX, PIN_RS485_TX, RS485_BAUD);
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
    st.valid = (startPos == 0x7F); // Simple validación
    st.lastMs = millis();
    return st;
  }

  void setDebug(bool on) { debugSerie = on; }

} // namespace RS485