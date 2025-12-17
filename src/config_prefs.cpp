#include <Preferences.h>
#include "config_prefs.hpp"
#include "definiciones.hpp"
#include "logBuf.hpp"


static Preferences prefs;
static const char* NS = "torno";
static const char* K_DCRC = "dcrc";   // uint32_t firma defaults firmware


bool cfgBegin() {
  return prefs.begin(NS, false);
}

static String getS(const char* k, const char* def) {
  return prefs.getString(k, def);
}
static int getI(const char* k, int def) {
  return prefs.getInt(k, def);
}

bool cfgValidateIP(const String& s) {
  int a,b,c,d;
  if (sscanf(s.c_str(), "%d.%d.%d.%d", &a,&b,&c,&d) != 4) return false;
  if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return false;
  return true;
}

bool cfgParseIP(const String& s, IPAddress& out) {
  int a,b,c,d;
  if (sscanf(s.c_str(), "%d.%d.%d.%d", &a,&b,&c,&d) != 4) return false;
  if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return false;
  out = IPAddress((uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d);
  return true;
}

static uint32_t fnv1a(const String& s) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < s.length(); ++i) { h ^= (uint8_t)s[i]; h *= 16777619u; }
  return h;
}

static uint32_t cfgDefaultsSignature()
{
  // Firma basada SOLO en los defaults hardcodeados (no en lo que haya en NVS)
  String s;
  s.reserve(200);

  s += DEVICE_ID; s += '|';
  s += sentido;   s += '|';
  s += String(aperturasTotales);   s += '|';
  s += String(modoRed); s += '|';

  s += IP.toString();      s += '|';
  s += GATEWAY.toString(); s += '|';
  s += SUBNET.toString();  s += '|';
  s += DNS1.toString();    s += '|';
  s += DNS2.toString();    s += '|';

  s += serverURL; s += '|';

  return fnv1a(s);
}


TornoConfig cfgLoad() {
  TornoConfig c;

  // Defaults = los que YA tienes hardcodeados hoy (así no rompes nada)
  c.deviceId = getS("id",  DEVICE_ID.c_str());
  c.sentido  = getS("sen", sentido.c_str());
  c.aperturasTotales = (uint32_t)getI("aperturasTotales", (int)aperturasTotales);
  c.modoRed  = (uint8_t)getI("modo", (int)modoRed);

  c.ip   = getS("ip",   IP.toString().c_str());
  c.gw   = getS("gw",   GATEWAY.toString().c_str());
  c.mask = getS("mask", SUBNET.toString().c_str());
  c.dns1 = getS("dns1", DNS1.toString().c_str());
  c.dns2 = getS("dns2", DNS2.toString().c_str());

  c.urlBase = getS("url", serverURL.c_str());
  return c;
}

static bool isValidDeviceId(const String& s) {
  if (s.length() < 3 || s.length() > 16) return false;
  // Ejemplo típico: ME001 / ME002...
  // Si quieres permitir cualquier cosa, quita esta validación.
  if (!s.startsWith("ME")) return false;
  return true;
}

static bool isValidUrlBase(const String& s) {
  if (s.length() < 8) return false;
  // Permite http y https
  if (!(s.startsWith("http://") || s.startsWith("https://"))) return false;
  return true;
}

bool cfgSave(const TornoConfig& c) {
  if (!isValidDeviceId(c.deviceId)) return false;
  if (!isValidUrlBase(c.urlBase)) return false;

  // sentido debe ser Entrada o Salida
  if (c.sentido != "Entrada" && c.sentido != "Salida") return false;

  // modoRed solo 0 o 1
  if (c.modoRed > 1) return false;

  // si estática, validar IPs
  if (c.modoRed == 1) {
    if (!cfgValidateIP(c.ip)) return false;
    if (!cfgValidateIP(c.gw)) return false;
    if (!cfgValidateIP(c.mask)) return false;
    if (!cfgValidateIP(c.dns1)) return false;
    if (!cfgValidateIP(c.dns2)) return false;
  }

  prefs.putString("id",   c.deviceId);
  prefs.putString("sen",  c.sentido);
  prefs.putInt("aperturasTotales", (int)c.aperturasTotales);
  prefs.putInt("modo",    (int)c.modoRed);

  prefs.putString("ip",   c.ip);
  prefs.putString("gw",   c.gw);
  prefs.putString("mask", c.mask);
  prefs.putString("dns1", c.dns1);
  prefs.putString("dns2", c.dns2);

  prefs.putString("url",  c.urlBase);
  return true;
}


void cfgApplyToGlobals(const TornoConfig& c) {
  DEVICE_ID = c.deviceId;
  sentido   = c.sentido;
  modoRed   = c.modoRed;
  serverURL = c.urlBase;

  // Si modo DHCP, puedes dejar 0.0.0.0 o lo que uses en tu begin()
  if (modoRed == 1) {
    cfgParseIP(c.ip,   IP);
    cfgParseIP(c.gw,   GATEWAY);
    cfgParseIP(c.mask, SUBNET);
    cfgParseIP(c.dns1, DNS1);
    cfgParseIP(c.dns2, DNS2);
  } else {
    // DHCP: estos valores no aplican hasta que Ethernet asigne IP real
    IP      = IPAddress(0,0,0,0);
    GATEWAY = IPAddress(0,0,0,0);
    SUBNET  = IPAddress(0,0,0,0);
    DNS1    = IPAddress(0,0,0,0);
    DNS2    = IPAddress(0,0,0,0);
  }
}

static TornoConfig cfgBuildDefaultsFromGlobals()
{
  TornoConfig d;
  d.deviceId = DEVICE_ID;
  d.sentido  = sentido;
  d.modoRed  = modoRed;

  d.ip   = IP.toString();
  d.gw   = GATEWAY.toString();
  d.mask = SUBNET.toString();
  d.dns1 = DNS1.toString();
  d.dns2 = DNS2.toString();

  d.urlBase = serverURL;
  return d;
}


void cfgEnsureFirmwareDefaults()
{
  const uint32_t now = cfgDefaultsSignature();
  const uint32_t saved = prefs.getUInt(K_DCRC, 0);

  // Caso 1: primera vez (no hay firma)
  // Caso 2: firmware nuevo (firma distinta) -> reset a defaults del firmware
  if (saved == 0 || saved != now)
  {
    TornoConfig d = cfgBuildDefaultsFromGlobals();
    cfgSave(d);                  // sobrescribe NVS con defaults actuales
    prefs.putUInt(K_DCRC, now);   // marca que este firmware ya “sembró” defaults
  }
}


