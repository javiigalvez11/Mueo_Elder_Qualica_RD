// W5500.cpp
#include <SPI.h>
#include <Ethernet.h>

#include "W5500.hpp"
#include "definiciones.hpp"

// ------- Helpers -------
static bool parseMacStr(const String &sIn, uint8_t out[6])
{
  String s = sIn;
  s.trim();
  s.toLowerCase();
  int b[6];
  if (sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6)
    return false;
  for (int i = 0; i < 6; i++)
    out[i] = (uint8_t)b[i];
  return true;
}

static void hwResetIfAvailable()
{
#if (ETH_RST_PIN >= 0)
  Serial.println(F("[ETH] Reset hardware W5500"));
  pinMode(ETH_RST_PIN, OUTPUT);
  digitalWrite(ETH_RST_PIN, LOW);
  delay(20);
  digitalWrite(ETH_RST_PIN, HIGH);
  delay(200);
#endif
}
namespace W5500
{
  void begin()
  {
    // 1) Dar tiempo a que los condensadores y la red eléctrica se estabilicen
    delay(500);

    hwResetIfAvailable();

    // 2) Asegurar que el pin CS esté arriba ANTES de iniciar SPI
    pinMode(ETH_CS_PIN, OUTPUT);
    digitalWrite(ETH_CS_PIN, HIGH);

    // 3) SPI primero
    SPI.begin(ETH_SCLK, ETH_MISO, ETH_MOSI, ETH_CS_PIN);

    // 4) Luego Ethernet (que usará el SPI ya iniciado)
    Ethernet.init(ETH_CS_PIN);
    delay(200);

    // --- CORRECCIÓN CRÍTICA AQUÍ ---
    // 5) PRIMERO esperamos a tener Link Físico (Cable conectado y negociado)
    Serial.println(F("[ETH] Esperando negociacion del puerto del router..."));
    uint32_t tLink = millis();
    // Le damos hasta 5 segundos al router para levantar el puerto
    while (Ethernet.linkStatus() == LinkOFF && (millis() - tLink) < 5000)
    {
      delay(100);
    }

    if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println(F("[ETH] ADVERTENCIA: No se detecta enlace físico (Cable desconectado o switch apagado)."));
    }
    else
    {
      Serial.println(F("[ETH] Enlace fisico OK."));
    }

    // 6) AHORA, y solo ahora, pedimos la IP
    if (modoRed == 0)
    {
      Serial.println(F("[ETH] Solicitando IP por DHCP..."));

      // Usamos los parámetros de Timeout (4000ms espera max, 2000ms respuesta)
      // para EVITAR que la placa se quede colgada 60 segundos si falla.
      int dhcpStatus = Ethernet.begin(MAC, 4000, 2000);

      if (dhcpStatus == 0)
      {
        Serial.println(F("[ETH] ERROR: Fallo al obtener IP por DHCP."));
      }
      else
      {
        IP = Ethernet.localIP();
        GATEWAY = Ethernet.gatewayIP();
        SUBNET = Ethernet.subnetMask();
        DNS1 = Ethernet.dnsServerIP();
      }
    }
    else
    {
      Serial.println(F("[ETH] Aplicando IP estatica..."));
      Ethernet.begin(MAC, IP, DNS1, GATEWAY, SUBNET);
    }

    delay(500);

    // 7) Diagnóstico final
    Serial.print(F("[ETH] MAC: "));
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                  MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

    auto hw = Ethernet.hardwareStatus();
    if (hw == EthernetNoHardware)
    {
      Serial.println(F("[ETH] ERROR FATAL: No se detecta W5500 por SPI"));
    }

    IPAddress lip = Ethernet.localIP();
    if (lip == IPAddress(0, 0, 0, 0) || lip == IPAddress(255, 255, 255, 255))
    {
      Serial.println(F("[ETH] RED NO DISPONIBLE (IP 0.0.0.0)"));
    }
    else
    {
      Serial.println(F("[ETH] INICIO DE RED EXITOSO"));
    }

    Serial.print(F("[ETH] IP:  "));
    Serial.println(lip);
    Serial.print(F("[ETH] GW:  "));
    Serial.println(Ethernet.gatewayIP());
    Serial.print(F("[ETH] MK:  "));
    Serial.println(Ethernet.subnetMask());
    Serial.print(F("[ETH] DNS: "));
    Serial.println(Ethernet.dnsServerIP());
  }
}