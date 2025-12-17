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
  delay(5);
  digitalWrite(ETH_RST_PIN, HIGH);
  delay(50);
#endif
}

void begin_ethernet()
{
  // 1) SPI primero
  SPI.begin(ETH_SCLK, ETH_MISO, ETH_MOSI, ETH_CS_PIN);
  Ethernet.init(ETH_CS_PIN);

  hwResetIfAvailable();
  delay(100);

  if (modoRed == 0)
  {
    // DHCP
    Ethernet.begin(mac);

    IP = Ethernet.localIP();
    GATEWAY = Ethernet.gatewayIP();
    SUBNET = Ethernet.subnetMask();
    DNS1 = Ethernet.dnsServerIP();
  }
  else
  {
    // IP fija
    Ethernet.begin(mac, IP, DNS1, GATEWAY, SUBNET);
  }

  // 5) Espera real a link (máx ~2.5s)
  uint32_t t0 = millis();
  while (Ethernet.linkStatus() == LinkOFF && (millis() - t0) < 2500)
  {
    delay(50);
  }

  delay(1000);

  // 6) Diagnóstico final
  Serial.print(F("[ETH] MAC: "));
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  auto hw = Ethernet.hardwareStatus();
  if (hw == EthernetNoHardware)
  {
    Serial.println(F("[ETH] ERROR: No se detecta W5500"));
  }

  IPAddress lip = Ethernet.localIP();
  if (lip == IPAddress(0, 0, 0, 0) || lip == IPAddress(255, 255, 255, 255))
  {
    Serial.println(F("[ETH] FALLÓ DHCP/IP (0.0.0.0 o 255.255.255.255)"));
  }
  else
  {
    Serial.println(F("[ETH] OK"));
  }

  Serial.print(F("[ETH] IP: "));
  Serial.println(lip);
  Serial.print(F("[ETH] GW: "));
  Serial.println(Ethernet.gatewayIP());
  Serial.print(F("[ETH] MK: "));
  Serial.println(Ethernet.subnetMask());
  Serial.print(F("[ETH] DNS: "));
  Serial.println(Ethernet.dnsServerIP());
}
