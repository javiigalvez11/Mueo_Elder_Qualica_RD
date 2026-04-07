#include <esp_wifi.h>
#include <WiFi.h>

#include "conexionWifi.hpp"
#include "definiciones.hpp"
#include "web_wifi.hpp" 
#include "logBuf.hpp"

// --- Evento de WiFi: La clave para capturar la IP por DHCP ---
void onWiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        // 1) Actualizamos las variables globales con lo que nos ha dado el router
        IP = WiFi.localIP();
        GATEWAY = WiFi.gatewayIP();
        SUBNET = WiFi.subnetMask();
        DNS1 = WiFi.dnsIP(0);
        DNS2 = WiFi.dnsIP(1);

        if (debugSerie) {
            Serial.print(F("[WiFi] DHCP OK! IP Asignada: "));
            Serial.println(IP.toString());
        }
        logbuf_pushf("[WiFi] Conectado. IP: %s", IP.toString().c_str());
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if (debugSerie) Serial.println(F("[WiFi] Desconectado. Reintentando..."));
        // No borramos la IP de las variables para que la web siga mostrando la "última conocida"
        // si así lo prefieres, o puedes ponerlas a 0 aquí.
        break;
        
    default:
        break;
    }
}

namespace WIFI
{
    void startFallbackAP() {
        Serial.println(F("[NET] Levantando modo Rescate (AP)..."));
        
        // Usamos AP_STA para que el AP funcione pero la antena siga buscando el WiFi principal de fondo
        WiFi.mode(WIFI_AP_STA); 
        WiFi.softAP(ssidAP.c_str(), passwordAP.c_str());
        
        IPAddress ipAP = WiFi.softAPIP();
        if (debugSerie) {
            Serial.print(F("[NET] AP Activo. IP: "));
            Serial.println(ipAP);
        }
        logbuf_pushf("[NET] Modo Rescate Activo. SSID: %s", ssidAP.c_str());
    }

    bool isConnected() {
        if (conexionRed == 0) return WiFi.status() == WL_CONNECTED;
        // Para Ethernet, el check es distinto:
        return (Ethernet.linkStatus() != LinkOFF && Ethernet.localIP() != IPAddress(0,0,0,0));
    }

    void reconnect() {
        if (WiFi.status() != WL_CONNECTED) {
            if (debugSerie) Serial.println(F("[NET] Forzando reconexión WiFi..."));
            WiFi.disconnect();
            WiFi.begin(ssidComercio.c_str(), passwordComercio.c_str());
        }
    }

    bool begin()
    {
        // 1) Configuración inicial de la radio
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);

        // Optimización de país y canal (opcional, ayuda en algunos entornos)
        wifi_country_t eu = {"EU", 1, 13, WIFI_COUNTRY_POLICY_AUTO};
        esp_wifi_set_country(&eu);

        // 2) Registrar el manejador de eventos ANTES de conectar
        WiFi.onEvent(onWiFiEvent);
        WiFi.setHostname(DEVICE_ID.c_str());

        // 3) Aplicar IP Estática solo si el modoRed es 1
        if (modoRed == 1) {
            if (debugSerie) Serial.println(F("[SETUP][WiFi] Configurando IP Estática..."));
            if (!WiFi.config(IP, GATEWAY, SUBNET, DNS1, DNS2)) {
                Serial.println(F("[SETUP][WiFi] Error configurando IP estática"));
            }
        } else {
            if (debugSerie) Serial.println(F("[SETUP][WiFi] Modo DHCP activo."));
        }

        // 4) Iniciar conexión
        if (debugSerie) Serial.printf("[SETUP][WiFi] Conectando a: %s\n", ssidComercio.c_str());
        logbuf_pushf("[SETUP][WiFi] Intentando conectar a %s", ssidComercio.c_str());
        
        WiFi.begin(ssidComercio.c_str(), passwordComercio.c_str());

        // 5) Servidores Web
        setupWebWiFi(); 
        serverWiFi.begin();
        
        return true; 
    }
} // namespace WIFI