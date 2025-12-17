#include "json.hpp"
#include "definiciones.hpp"
#include "rele.hpp"
#include "logBuf.hpp"

// ========================= Helpers internos =========================

static inline int toIntSafe(const String &s, int dflt)
{
  if (s.length() == 0) return dflt;
  char *end = nullptr;
  long v = strtol(s.c_str(), &end, 10);
  return (end && *end == '\0') ? (int)v : dflt;
}

static inline bool json_is_ok(const JsonDocument &d)
{
  String r = d["r"] | "";
  r.trim(); r.toLowerCase();
  return (r == "ok" || r == "success");
}

static inline int get_status_as_int(const JsonDocument &d)
{
  if (d["status"].is<int>()) return (int)d["status"];
  String s = d["status"] | "";
  return toIntSafe(s, 0);
}

static inline String compose_error_detail(const JsonDocument &d)
{
  String ec = d["ec"] | "";
  String rs = d["reason"] | "";
  String ed = d["ed"] | "";

  if (ec.length() && rs.length()) return ec + ":" + rs;
  if (ec.length()) return ec;
  if (rs.length()) return rs;
  if (ed.length()) return ed;
  return "UNKNOWN_ERROR";
}

// ========================= LOG helpers =========================

static void log_line(const char *fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  logbuf_pushf("%s", buf);
  if (debugSerie) Serial.println(buf);
}

static String oneLine(String s)
{
  s.replace("\r", "");
  s.replace("\n", "");
  s.trim();
  return s;
}

static void log_payload(const char *prefix, const String &payload)
{
  log_line("%s bytes=%u", prefix, payload.length());
  String s = oneLine(payload);

  const size_t CHUNK = 160;
  for (size_t i = 0; i < s.length(); i += CHUNK)
    log_line("%s", s.substring(i, min(i + CHUNK, s.length())).c_str());
}

// ========================= Feedback =========================

static inline void feedbackError(const String &ed)
{
  if (debugSerie)
  {
    Serial.print("[IO][ERROR] ");
    Serial.println(ed);
  }
}

static void aplicarAperturaContinua(bool on)
{
  int nuevo = on ? 1 : 0;
  if (abreContinua == nuevo) return;

  abreContinua = nuevo;
  if (abreContinua)
  {
    if (debugSerie) Serial.println("[IO] Apertura continua: ON");
    rele_open();
  }
  else
  {
    if (debugSerie) Serial.println("[IO] Apertura continua: OFF");
    rele_close();
  }
}

void resetCycleReady()
{
  activaEntrada = 0;
  activaSalida = 0;
  activaConecta = 1;
  ultimoTicket = "";
  ultimoPaso = "";
  pasoActual = 0;
  contadorPasos = 0;
  estadoMaquina = CMD_READY;
  estadoPuerta = "200";
  abortarPaso = 0;
  aplicarAperturaContinua(false);
}

// ========================= EC helpers =========================

static inline const char *ec_to_str(CmdType c)
{
  switch (c)
  {
    case CMD_READY: return "CMD_READY";
    case CMD_OPEN_CONTINUOUS: return "CMD_OPEN_CONTINUOUS";
    case CMD_VALIDATE_IN: return "CMD_VALIDATE_IN";
    case CMD_VALIDATE_OUT: return "CMD_VALIDATE_OUT";
    case CMD_PASS_OK: return "CMD_PASS_OK";
    case CMD_PASS_TIMEOUT: return "CMD_PASS_TIMEOUT";
    case CMD_PASSED_IN: return "CMD_PASSED_IN";
    case CMD_PASSED_OUT: return "CMD_PASSED_OUT";
    case CMD_RESTART: return "CMD_RESTART";
    case CMD_UPDATE: return "CMD_UPDATE";
    default: return "CMD_NONE";
  }
}

static CmdType cmd_from_ec_string(const String &ec)
{
  if (ec == "CMD_READY") return CMD_READY;
  if (ec == "CMD_OPEN_CONTINUOUS") return CMD_OPEN_CONTINUOUS;
  if (ec == "CMD_VALIDATE_IN") return CMD_VALIDATE_IN;
  if (ec == "CMD_VALIDATE_OUT") return CMD_VALIDATE_OUT;
  if (ec == "CMD_PASS_OK") return CMD_PASS_OK;
  if (ec == "CMD_PASS_TIMEOUT") return CMD_PASS_TIMEOUT;
  if (ec == "CMD_PASSED_IN") return CMD_PASSED_IN;
  if (ec == "CMD_PASSED_OUT") return CMD_PASSED_OUT;
  if (ec == "CMD_RESTART") return CMD_RESTART;
  if (ec == "CMD_UPDATE") return CMD_UPDATE;
  return CMD_NONE;
}

// ========================= Aplicar estado =========================

static void applyStatus(int status, const String &ec, int nt, int np, const char *from)
{
  CmdType nuevo = cmd_from_ec_string(ec);
  if (nuevo == CMD_NONE)
  {
    if (status == 200) nuevo = CMD_READY;
    else if (status == 201) nuevo = CMD_VALIDATE_IN;
    else if (status == 202) nuevo = CMD_VALIDATE_OUT;
  }

  estadoPuerta = String(status);
  estadoMaquina = nuevo;

  if (debugSerie)
  {
    Serial.print("[API][");
    Serial.print(from);
    Serial.print("] ");
    Serial.print(status);
    Serial.print(" ec=");
    Serial.println(ec);
  }
}

// ========================= Serializadores =========================
void serializaInicio()
{
  JsonDocument doc;
  doc["r"] = "OK";
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;

  outputInicio.remove(0);
  serializeJson(doc, outputInicio);

  log_payload("[HTTP][OUT][inicio]", outputInicio);
}
void serializaEstado()
{
  JsonDocument doc;
  doc["r"] = "OK";
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;
  doc["ec"] = ec_to_str(estadoMaquina);

  outputEstado.remove(0);
  serializeJson(doc, outputEstado);

  log_payload("[HTTP][OUT][estado]", outputEstado);
}

void serializaQR()
{
  JsonDocument doc;
  doc["r"] = "OK";
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;
  doc["ec"] = ec_to_str(estadoMaquina);
  doc["barcode"] = ultimoTicket;

  outputTicket.remove(0);
  serializeJson(doc, outputTicket);

  log_payload("[HTTP][OUT][qr]", outputTicket);
}

void serializaPaso()
{
  JsonDocument doc;
  doc["r"] = "OK";
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;
  doc["ec"] = ec_to_str(estadoMaquina);
  doc["barcode"] = ultimoPaso;
  doc["np"] = pasoActual;
  doc["nt"] = pasosTotales;

  outputPaso.remove(0);
  serializeJson(doc, outputPaso);

  log_payload("[HTTP][OUT][pass]", outputPaso);
}

// ========================= Deserializadores =========================

void descifraEstado()
{
  log_payload("[HTTP][IN][estado]", estadoRecibido);

  JsonDocument doc;
  if (deserializeJson(doc, estadoRecibido)) return;

  if (json_is_ok(doc))
    applyStatus(get_status_as_int(doc), doc["ec"] | "", 0, 0, "estado");

  estadoRecibido = "";
}

void descifraQR()
{
  log_payload("[HTTP][IN][qr]", ticketRecibido);

  JsonDocument doc;
  if (deserializeJson(doc, ticketRecibido)) return;

  if (json_is_ok(doc))
    applyStatus(get_status_as_int(doc), doc["ec"] | "", 1, 0, "qr");

  ticketRecibido = "";
}

void descifraPaso()
{
  log_payload("[HTTP][IN][pass]", pasoRecibido);

  JsonDocument doc;
  if (deserializeJson(doc, pasoRecibido)) return;

  if (json_is_ok(doc))
    applyStatus(get_status_as_int(doc), doc["ec"] | "", pasosTotales, contadorPasos, "pass");

  pasoRecibido = "";
}
