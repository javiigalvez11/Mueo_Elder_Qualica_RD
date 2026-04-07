#include <Preferences.h>
#include "config_prefs.hpp"
#include "definiciones.hpp"
#include "logBuf.hpp"

static Preferences prefs;
static const char *NS = "torno";
static const char *K_DCRC = "dcrc"; // Firma de los defaults

bool cfgBegin()
{
  return prefs.begin(NS, false);
}

// ========================= Auxiliares =========================
static String getS(const char *k, const char *def)
{
  return prefs.getString(k, def);
}
static int getI(const char *k, int def)
{
  return prefs.getInt(k, def);
}

bool cfgValidateIP(const String &s)
{
  int a, b, c, d;
  if (sscanf(s.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    return false;
  if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255)
    return false;
  return true;
}

bool cfgParseIP(const String &s, IPAddress &out)
{
  int a, b, c, d;
  if (sscanf(s.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
    return false;
  out = IPAddress((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d);
  return true;
}

static uint32_t fnv1a(const String &s)
{
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < s.length(); ++i)
  {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h;
}

// ========================= Lógica de Firma =========================
static uint32_t cfgDefaultsSignature()
{
  String s;
  s.reserve(300);
  s += DEVICE_ID;
  s += '|';
  for (int i = 0; i < 6; i++)
    s += String(MAC[i], HEX);
  s += '|';
  s += serverURL;
  s += '|';
  s += '|';
  s += urlActualiza;
  s += '|';
  s += String(modoPasillo);
  s += '|';
  s += String(modoApertura);
  s += '|';
  s += String(sentidoApertura);
  s += '|';
  s += String(conexionRed);
  s += '|';
  s += ssidComercio;
  s += '|';
  s += passwordComercio;
  s += '|';
  s += String(modoRed);
  s += '|';
  s += IP.toString();
  s += '|';
  return fnv1a(s);
}

// ========================= Carga y Guardado =========================
TornoConfig cfgLoad()
{
  TornoConfig c;
  c.deviceId = getS("id", DEVICE_ID.c_str());

  // Cargamos la MAC desde bytes (6 bytes fijos)
  size_t len = prefs.getBytes("mac", c.mac, 6);
  if (len != 6)
  { // Si no hay MAC guardada, usamos la de definiciones.hpp
    for (int i = 0; i < 6; i++)
      c.mac[i] = MAC[i];
  }

  // IMPORTANTE: Claves unificadas con cfgSave
  c.urlBase = getS("urlBase", serverURL.c_str());
  c.urlActualiza = getS("urlActualiza", urlActualiza.c_str());

  c.modoPasillo = (uint8_t)getI("modoPasillo", (int)modoPasillo);
  c.modoApertura = (uint32_t)getI("modoApertura", (int)modoApertura);
  c.sentidoApertura = (uint32_t)getI("sentidoApertura", (int)sentidoApertura);
  c.entradasTotales = (uint32_t)getI("entradasTotales", (int)entradasTotales);
  c.salidasTotales = (uint32_t)getI("salidasTotales", (int)salidasTotales);

  c.conexionRed = (uint8_t)getI("conexionRed", (int)conexionRed);
  c.wifiSSID = getS("wifiSSID", ssidComercio.c_str());
  c.wifiPass = getS("wifiPass", passwordComercio.c_str());
  c.modoRed = (uint8_t)getI("modoRed", (int)modoRed);
  c.ip = getS("ip", IP.toString().c_str());
  c.gw = getS("gw", GATEWAY.toString().c_str());
  c.mask = getS("mask", SUBNET.toString().c_str());
  c.dns1 = getS("dns1", DNS1.toString().c_str());
  c.dns2 = getS("dns2", DNS2.toString().c_str());

  return c;
}

bool cfgSave(const TornoConfig &c)
{
  // Validaciones básicas
  if (c.deviceId.length() < 3)
    return false;
  if (!c.urlBase.startsWith("http"))
    return false;

  prefs.putString("id", c.deviceId);
  prefs.putBytes("mac", c.mac, 6); // Guardamos 6 bytes del array

  prefs.putString("urlBase", c.urlBase);
  prefs.putString("urlActualiza", c.urlActualiza);

  prefs.putInt("modoPasillo", (int)c.modoPasillo);
  prefs.putInt("modoApertura", (int)c.modoApertura);
  prefs.putInt("sentidoApertura", (int)c.sentidoApertura);
  prefs.putInt("entradasTotales", (int)c.entradasTotales);
  prefs.putInt("salidasTotales", (int)c.salidasTotales);

  prefs.putInt("conexionRed", (int)c.conexionRed);
  prefs.putString("wifiSSID", c.wifiSSID);
  prefs.putString("wifiPass", c.wifiPass);
  prefs.putInt("modoRed", (int)c.modoRed);
  prefs.putString("ip", c.ip);
  prefs.putString("gw", c.gw);
  prefs.putString("mask", c.mask);
  prefs.putString("dns1", c.dns1);
  prefs.putString("dns2", c.dns2);

  return true;
}

// ========================= Aplicación =========================
void cfgApplyToGlobals(const TornoConfig &c)
{
  DEVICE_ID = c.deviceId;
  for (int i = 0; i < 6; i++)
    MAC[i] = c.mac[i];

  serverURL = c.urlBase;
  urlActualiza = c.urlActualiza;

  modoPasillo = c.modoPasillo;
  modoApertura = c.modoApertura;
  sentidoApertura = c.sentidoApertura;
  entradasTotales = c.entradasTotales;
  salidasTotales = c.salidasTotales;

  conexionRed = c.conexionRed;
  ssidComercio = c.wifiSSID;
  passwordComercio = c.wifiPass;
  modoRed = c.modoRed;

  if (modoRed == 1)
  {
    cfgParseIP(c.ip, IP);
    cfgParseIP(c.gw, GATEWAY);
    cfgParseIP(c.mask, SUBNET);
    cfgParseIP(c.dns1, DNS1);
    cfgParseIP(c.dns2, DNS2);
  }
  else
  {
    IP = GATEWAY = SUBNET = DNS1 = DNS2 = IPAddress(0, 0, 0, 0);
  }
}

static TornoConfig cfgBuildDefaultsFromGlobals()
{
  TornoConfig d;
  d.deviceId = DEVICE_ID;
  for (int i = 0; i < 6; i++)
    d.mac[i] = MAC[i];

  d.urlBase = serverURL;
  d.urlActualiza = urlActualiza;
  d.modoPasillo = modoPasillo;
  d.modoApertura = modoApertura;
  d.sentidoApertura = sentidoApertura;
  d.entradasTotales = entradasTotales;
  d.salidasTotales = salidasTotales;
  d.conexionRed = conexionRed;
  d.wifiSSID = ssidComercio;
  d.wifiPass = passwordComercio;
  d.modoRed = modoRed;
  d.ip = IP.toString();
  d.gw = GATEWAY.toString();
  d.mask = SUBNET.toString();
  d.dns1 = DNS1.toString();
  d.dns2 = DNS2.toString();
  return d;
}

void cfgEnsureFirmwareDefaults()
{
  uint32_t now = cfgDefaultsSignature();
  uint32_t saved = prefs.getUInt(K_DCRC, 0);

  if (saved == 0 || saved != now)
  {
    logbuf_pushf("Config: Detectado nuevo firmware o NVS vacía. Aplicando defaults.");
    TornoConfig d = cfgBuildDefaultsFromGlobals();
    if (cfgSave(d))
    {
      prefs.putUInt(K_DCRC, now);
    }
  }
}