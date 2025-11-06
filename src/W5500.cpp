#include "W5500.hpp"
#include "config.hpp"
#include <SPI.h>
#include <Ethernet.h>

// MAC por defecto (cámbiala por equipo)
uint8_t g_mac[6] = { 0x02, 0x11, 0x0A, 0x6E, 0x01, 0x03 }; // 02-LOCAL | 10.110.1.203

void begin_ethernet() {
  // Arranca bus SPI y chip W5500
  SPI.begin(ETH_SCK, ETH_MISO, ETH_MOSI, ETH_CS_PIN);
  Ethernet.init(ETH_CS_PIN);

#if USE_STATIC_IP
  // Orden correcto: (mac, ip, dns, gateway, subnet)
  Ethernet.begin(g_mac, LOCAL_IP, DNS1, GATEWAY, SUBNET);
#else
  // DHCP (algunas variantes devuelven void)
  Ethernet.begin(g_mac);
#endif

  if (Ethernet.localIP() == IPAddress(0,0,0,0)) {
    Serial.println(F("[ETH] FALLÓ red (IP 0.0.0.0)"));
  } else {
    Serial.println(F("[ETH] OK"));
  }

  Serial.print(F("[ETH] IP: "));   Serial.println(Ethernet.localIP());
  Serial.print(F("[ETH] GW: "));   Serial.println(Ethernet.gatewayIP());
  Serial.print(F("[ETH] MK: "));   Serial.println(Ethernet.subnetMask());
  Serial.print(F("[ETH] DNS: "));  Serial.println(Ethernet.dnsServerIP());
}
