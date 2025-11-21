#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <IPAddress.h>
#include "archivos.hpp"
#include "definiciones.hpp"
#include "W5500.hpp"

// --------- Defaults (por si no hay config.json) ----------
static const char *DEF_SSID = "ME001_WIFI";
static const char *DEF_PASS = "Elder-01";
static const char *DEF_MASTER = "Qualicard";
static const char *DEF_DEVICE_ID = "ME001";
static const char *DEF_SENTIDO = "Entrada";
static const char *DEF_URL_BASE = "http://10.110.1.9:8084/PTService/ESP32";
static const char *DEF_URL_OTA = "http://10.110.1.9:8084/firmware/ME001.bin"; // HTTP para W5500
static const uint8_t DEF_MODORED = 0;

// ================= Helpers JSON v7 =================
static inline bool hasCStr(const JsonDocument &d, const char *key)
{
  return d[key].is<const char *>();
}
static inline String getStrOr(const JsonDocument &d, const char *key, const char *dflt)
{
  return hasCStr(d, key) ? String(d[key].as<const char *>()) : String(dflt);
}
static inline uint8_t getU8Or(const JsonDocument &d, const char *key, uint8_t dflt)
{
  if (d[key].is<uint8_t>())
    return d[key].as<uint8_t>();
  if (d[key].is<int>())
    return (uint8_t)d[key].as<int>();
  if (d[key].is<const char *>())
    return (uint8_t)atoi(d[key].as<const char *>());
  return dflt;
}
static bool parseIP(const char *s, IPAddress &out)
{
  if (!s || !*s)
    return false;
  return out.fromString(s);
}
static bool parseMAC(const char *s, uint8_t out[6])
{
  // Acepta "1c:69:20:e6:21:78" o "1C-69-20-E6-21-78"
  if (!s)
    return false;
  char buf[32];
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  for (char *p = buf; *p; ++p)
  {
    if (*p == '-')
      *p = ':';
  } // normaliza a ':'

  int values[6];
  if (sscanf(buf, "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6)
  {
    return false;
  }
  for (int i = 0; i < 6; i++)
    out[i] = (uint8_t)values[i];
  return true;
}

// =============== crear config.json por defecto si no existe ===============
static void ensureConfigExists()
{
  if (LittleFS.exists("/config.json"))
    return;

  if (debugSerie)
    Serial.println(F("[CFG] /config.json no existe → creando por defecto"));
  File f = LittleFS.open("/config.json", FILE_WRITE);
  if (!f)
  {
    if (debugSerie)
      Serial.println(F("[CFG] ERROR: no se pudo crear /config.json"));
    return;
  }
  JsonDocument doc;
  doc["ssid"] = DEF_SSID;
  doc["password"] = DEF_PASS;
  doc["credencialMaestra"] = DEF_MASTER;
  doc["nombrePlaca"] = DEF_DEVICE_ID;
  doc["mac"] = "1c:69:20:e6:21:78";
  doc["sentido"] = DEF_SENTIDO;
  doc["urlBase"] = DEF_URL_BASE;
  doc["urlActualizar"] = DEF_URL_OTA;
  doc["modoRed"] = DEF_MODORED; // 0=DHCP, 1=IP fija
  doc["ip"] = "10.110.1.203";
  doc["gateway"] = "10.110.1.254";
  doc["subnet"] = "255.255.255.0";
  doc["dns1"] = "8.8.8.8";
  doc["dns2"] = "4.4.4.4";
  serializeJsonPretty(doc, f);
  f.close();
}

// =============== API pública: carga configuración ===============
void cargaConfig()
{
  // Montar FS si no está
  if (!LittleFS.begin(false))
  {
    if (debugSerie)
      Serial.println(F("[FS] LittleFS mFount falló → intentando formateo"));
    if (!LittleFS.begin(true))
    {
      if (debugSerie)
        if (debugSerie)
          Serial.println(F("[FS] LittleFS no disponible"));
      // Defaults mínimos de runtime
      ssid = DEF_SSID;
      password = DEF_PASS;
      credencialMaestra = DEF_MASTER;
      DEVICE_ID = DEF_DEVICE_ID;
      serverURL = DEF_URL_BASE;
      urlActualiza = DEF_URL_OTA;
      modoRed = DEF_MODORED;
      sentido = DEF_SENTIDO;
      IP = IPAddress(0, 0, 0, 0);
      GATEWAY = IPAddress(0, 0, 0, 0);
      SUBNET = IPAddress(0, 0, 0, 0);
      DNS1 = IPAddress(0, 0, 0, 0);
      DNS2 = IPAddress(0, 0, 0, 0);
      // MAC por defecto (si procede)
      mac[0] = 0x1C;
      mac[1] = 0x69;
      mac[2] = 0x20;
      mac[3] = 0xE6;
      mac[4] = 0x21;
      mac[5] = 0x78;
      return;
    }
  }

  // Crear config por defecto si falta
  ensureConfigExists();

  // Abrir config.json
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile)
  {
    if (debugSerie)
      Serial.println(F("[CFG] /config.json no existe (usando defaults)"));
    ssid = DEF_SSID;
    password = DEF_PASS;
    credencialMaestra = DEF_MASTER;
    DEVICE_ID = DEF_DEVICE_ID;
    serverURL = DEF_URL_BASE;
    urlActualiza = DEF_URL_OTA;
    modoRed = DEF_MODORED;
    sentido = DEF_SENTIDO;
    IP = IPAddress(0, 0, 0, 0);
    GATEWAY = IPAddress(0, 0, 0, 0);
    SUBNET = IPAddress(0, 0, 0, 0);
    DNS1 = IPAddress(0, 0, 0, 0);
    DNS2 = IPAddress(0, 0, 0, 0);
    mac[0] = 0x1C;
    mac[1] = 0x69;
    mac[2] = 0x20;
    mac[3] = 0xE6;
    mac[4] = 0x21;
    mac[5] = 0x78;
    return;
  }

  // Parsear JSON (v7)
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, configFile);
  configFile.close();
  if (err)
  {
    if (debugSerie)
    {
      Serial.print(F("[CFG] Error JSON: "));
      Serial.println(err.c_str());
    }
    // Defaults si el archivo está corrupto
    ssid = DEF_SSID;
    password = DEF_PASS;
    credencialMaestra = DEF_MASTER;
    DEVICE_ID = DEF_DEVICE_ID;
    serverURL = DEF_URL_BASE;
    urlActualiza = DEF_URL_OTA;
    modoRed = DEF_MODORED;
    sentido = DEF_SENTIDO;
    IP = IPAddress(0, 0, 0, 0);
    GATEWAY = IPAddress(0, 0, 0, 0);
    SUBNET = IPAddress(0, 0, 0, 0);
    DNS1 = IPAddress(0, 0, 0, 0);
    DNS2 = IPAddress(0, 0, 0, 0);
    mac[0] = 0x1C;
    mac[1] = 0x69;
    mac[2] = 0x20;
    mac[3] = 0xE6;
    mac[4] = 0x21;
    mac[5] = 0x78;
    return;
  }

  // Strings básicas
  ssid = getStrOr(doc, "ssid", DEF_SSID);
  password = getStrOr(doc, "password", DEF_PASS);
  credencialMaestra = getStrOr(doc, "credencialMaestra", DEF_MASTER);
  DEVICE_ID = getStrOr(doc, "nombrePlaca", DEF_DEVICE_ID);

  // URLs
  serverURL = getStrOr(doc, "urlBase", DEF_URL_BASE);
  urlActualiza = getStrOr(doc, "urlActualizar", DEF_URL_OTA);

  // Red
  modoRed = getU8Or(doc, "modoRed", DEF_MODORED); // 0=DHCP, 1=IP fija
  sentido = getStrOr(doc, "sentido", DEF_SENTIDO);

  // IPs
  IP = IPAddress(0, 0, 0, 0);
  GATEWAY = IPAddress(0, 0, 0, 0);
  SUBNET = IPAddress(0, 0, 0, 0);
  DNS1 = IPAddress(0, 0, 0, 0);
  DNS2 = IPAddress(0, 0, 0, 0);
  (void)parseIP(doc["ip"].as<const char *>(), IP);
  (void)parseIP(doc["gateway"].as<const char *>(), GATEWAY);
  (void)parseIP(doc["subnet"].as<const char *>(), SUBNET);
  (void)parseIP(doc["dns1"].as<const char *>(), DNS1);
  (void)parseIP(doc["dns2"].as<const char *>(), DNS2);

  // MAC
  // Si no se puede parsear, deja la default (coincidente con tu JSON)
  uint8_t macTmp[6] = {0x1C, 0x69, 0x20, 0xE6, 0x21, 0x78};
  if (hasCStr(doc, "mac"))
  {
    const char *s = doc["mac"].as<const char *>();
    if (parseMAC(s, macTmp))
    {
      memcpy(mac, macTmp, 6);
    }
    else
    {
      memcpy(mac, macTmp, 6);
    }
  }
  else
  {
    memcpy(mac, macTmp, 6);
  }

  // Log
  if (debugSerie)
  {
    Serial.println(F("[CFG] Cargada configuración:"));
    Serial.print(F("  DEVICE_ID: "));
    Serial.println(DEVICE_ID);
    Serial.print(F("  ssid: "));
    Serial.println(ssid);
    Serial.print(F("  urlBase: "));
    Serial.println(serverURL);
    Serial.print(F("  urlActualizar: "));
    Serial.println(urlActualiza);
    Serial.print(F("  modoRed: "));
    Serial.println(modoRed);
    Serial.print(F("  sentido: "));
    Serial.println(sentido);
    Serial.print(F("  MAC: "));
    for (int i = 0; i < 6; i++)
    {
      if (i)
        Serial.print(':');
      if (mac[i] < 16)
        Serial.print('0');
      Serial.print(mac[i], HEX);
    }
    Serial.println();
    Serial.print(F("  IP: "));
    Serial.println(IP);
    Serial.print(F("  GW: "));
    Serial.println(GATEWAY);
    Serial.print(F("  MK: "));
    Serial.println(SUBNET);
    Serial.print(F("  DNS1: "));
    Serial.println(DNS1);
    Serial.print(F("  DNS2: "));
    Serial.println(DNS2);
  }
}
