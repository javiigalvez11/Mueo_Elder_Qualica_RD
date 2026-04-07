#include <SPI.h>
#include <Ethernet.h>

#include "W5500.hpp"
#include "definiciones.hpp"
#include "web_eth.hpp" // Para serverETH
#include "logBuf.hpp"
#include "conexionWifi.hpp" // Para fallback AP
#include "web_wifi.hpp" // Para cargar la configuración de red

namespace W5500
{
    void begin()
    {
        // 1) SPI e inicialización
        SPI.begin(ETH_SCLK, ETH_MISO, ETH_MOSI, ETH_CS_PIN);
        Ethernet.init(ETH_CS_PIN);

        // Reset por hardware si el pin está definido
#if (ETH_RST_PIN >= 0)
        Serial.println(F("[ETH] Reset hardware W5500"));
        pinMode(ETH_RST_PIN, OUTPUT);
        digitalWrite(ETH_RST_PIN, LOW);
        delay(20);
        digitalWrite(ETH_RST_PIN, HIGH);
        delay(200);
#endif

        delay(100);

        // 2) Selección de modo de Red
        if (modoRed == 0)
        {
            Serial.println(F("[ETH] Intentando DHCP..."));
            // Ethernet.begin devuelve 1 si tiene éxito
            if (Ethernet.begin(MAC, 10000, 4000) == 0)
            { // Timeout 10s, Response 4s
                Serial.println(F("[ETH] DHCP falló. Activando AP de rescate..."));
                WIFI::startFallbackAP(); // <--- Lanzamos el AP si el cable/router falla
                setupWebWiFi();          // <--- IMPORTANTE: Iniciamos handlers de WiFi
                serverWiFi.begin();
                return;
            }
        }
        else
        {
            Ethernet.begin(MAC, IP, DNS1, GATEWAY, SUBNET);
        }

        // 3) Espera de Link (máx 2.5s)
        uint32_t t0 = millis();
        while (Ethernet.linkStatus() == LinkOFF && (millis() - t0) < 2500)
        {
            delay(50);
        }

        // 4) Diagnóstico de Hardware
        auto hw = Ethernet.hardwareStatus();
        if (hw == EthernetNoHardware)
        {
            Serial.println(F("[SETUP][ETH] ERROR: No se detecta W5500"));
            logbuf_pushf("[SETUP][ETH] ERROR: No se detecta W5500");
            return;
        }

        IPAddress lip = Ethernet.localIP();
        if (lip == IPAddress(0, 0, 0, 0) || lip == IPAddress(255, 255, 255, 255))
        {
            Serial.println(F("[SETUP][ETH] FALLÓ asignación de IP"));
            logbuf_pushf("[SETUP][ETH] FALLÓ asignación de IP");
        }
        else
        {
            Serial.println(F("[SETUP][ETH] Conexión establecida OK"));
            logbuf_pushf("[SETUP][ETH] Conexión establecida OK");

            // --- ARRANQUE DEL SERVIDOR ---
            serverETH.begin();
            Serial.println(F("[SETUP][ETH] Servidor Ethernet iniciado"));
        }

        // Logs finales de estado
        Serial.print(F("[SETUP][ETH] IP: "));
        Serial.println(lip);
        Serial.print(F("[SETUP][ETH] GW: "));
        Serial.println(Ethernet.gatewayIP());
        Serial.print(F("[SETUP][ETH] DNS: "));
        Serial.println(Ethernet.dnsServerIP());

        logbuf_pushf("[SETUP][ETH] IP: %s", lip.toString().c_str());
        logbuf_pushf("[SETUP][ETH] GW: %s", Ethernet.gatewayIP().toString().c_str());
        logbuf_pushf("[SETUP][ETH] DNS: %s", Ethernet.dnsServerIP().toString().c_str());
    }

    bool reintentarDHCP() {
        Serial.println(F("[ETH] Reintentando obtener IP por DHCP..."));
        // Intentamos obtener IP (timeout reducido para no bloquear la tarea)
        if (Ethernet.begin(MAC, 5000, 2000) != 0) {
            if(debugSerie) {
            Serial.print(F("[ETH] IP Obtenida: "));
            Serial.println(Ethernet.localIP());    
            }

            logbuf_pushf("[ETH] IP Obtenida: %s", Ethernet.localIP().toString().c_str());

            serverETH.begin(); // Iniciamos el servidor si tuvimos éxito
            return true;
        }
        Serial.println(F("[ETH] Reintento fallido."));
        return false;
    }
} // namespace W5500