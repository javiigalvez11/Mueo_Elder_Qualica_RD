#include "json.hpp"
#include "config.hpp"
#include "rele.hpp"   // para rele_open/rele_close en apertura continua

// ========================= Helpers internos =========================

static inline int toIntSafe(const String& s, int dflt) {
  if (s.length() == 0) return dflt;
  char* end = nullptr;
  long v = strtol(s.c_str(), &end, 10);
  return (end && *end == '\0') ? (int)v : dflt;
}

// Normaliza 'r' leyendo "r" o "R"
static bool jsonRok(JsonDocument& doc) {
  String r;
  if (!doc["r"].isNull())      r = doc["r"].as<String>();
  else if (!doc["R"].isNull()) r = doc["R"].as<String>();
  else                         r = "";
  r.trim(); r.toLowerCase();
  return r == "ok";
}

// Acciones visuales/sonoras opcionales
static inline void feedbackError(const String& ed) {
  if (debugSerie) { Serial.print("[IO][ERROR] "); Serial.println(ed); }
}

// Pequeño helper para toggle de apertura continua
static void aplicarAperturaContinua(bool on) {
  int nuevo = on ? 1 : 0;
  if (abreContinua == nuevo) return;
  abreContinua = nuevo;
  if (abreContinua) {
    if (debugSerie) Serial.println("[IO] Apertura continua: ON");
    rele_open();
  } else {
    if (debugSerie) Serial.println("[IO] Apertura continua: OFF");
    rele_close();
  }
}

// Resetea el ciclo a READY con puerta 200
static void resetCycleReady() {
  activaEntrada = 0;
  activaSalida  = 0;
  activaConecta = 1;
  ultimoTicket  = "";
  pasoActual    = 0;
  contadorPasos = 0;
  estadoMaquina = "CMD_READY";
  estadoPuerta  = "200";
  abortarPaso   = 0;
  aplicarAperturaContinua(false);
}

// Maneja cualquier estado de puerta de forma centralizada.
static void applyStatus(int status, const String& ec, int nt, int np, const char* from) {
  switch (status) {
    case 200: { // READY / heartbeat normal
      resetCycleReady();
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from); Serial.println("] 200 READY");
        if (ec.length()) { Serial.print("  ec="); Serial.println(ec); }
      }
    } break;

    case 201: { // ENTRADA autorizada
      activaConecta = 0;
      pasosTotales  = (nt > 0) ? nt : 1;
      contadorPasos = (np >= 0) ? np : 0;
      activaEntrada = 1;
      estadoMaquina = "CMD_VALIDATE_IN";
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from);
        Serial.print("] 201 ENTRADA: nt="); Serial.print(pasosTotales);
        Serial.print(" np="); Serial.print(contadorPasos);
        if (ec.length()) { Serial.print(" ec="); Serial.print(ec); }
        Serial.println();
      }
    } break;

    case 202: { // SALIDA autorizada (si la usáis)
      activaConecta = 0;
      pasosTotales  = (nt > 0) ? nt : 1;
      contadorPasos = (np >= 0) ? np : 0;
      activaSalida  = 1;
      estadoMaquina = "CMD_VALIDATE_OUT";
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from);
        Serial.print("] 202 SALIDA: nt="); Serial.print(pasosTotales);
        Serial.print(" np="); Serial.print(contadorPasos);
        if (ec.length()) { Serial.print(" ec="); Serial.print(ec); }
        Serial.println();
      }
    } break;

    case 203: { // APERTURA CONTINUA (mantener abierta)
      estadoPuerta  = "203";
      estadoMaquina = "CMD_OPEN_CONTINUOUS";
      aplicarAperturaContinua(true);   // ← CORREGIDO: activar (antes estaba false)
      activaConecta = 1;
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from); Serial.print("] 203 APERTURA CONTINUA");
        if (ec.length()) { Serial.print(" ec="); Serial.print(ec); }
        Serial.println();
      }
    } break;

    case 300: { // CANCELAR/ABORTAR paso
      estadoPuerta = "300";
      abortarPaso  = 1;
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from); Serial.print("] 300 CANCELAR");
        if (ec.length()) { Serial.print(" ec="); Serial.print(ec); }
        Serial.println();
      }
    } break;

    case 310: { // UPDATE OTA
      actualizarFlag = 1;
      resetCycleReady();
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from); Serial.print("] 310 UPDATE");
        if (ec.length()) { Serial.print(" ec="); Serial.print(ec); }
        Serial.println();
      }
    } break;

    default: {  // 4xx/5xx u otros
      if (debugSerie) {
        Serial.print("[API]["); Serial.print(from); Serial.print("] status=");
        Serial.print(status);
        if (ec.length()) { Serial.print(" ec="); Serial.print(ec); }
        Serial.println();
      }
      // No cambiamos flags de entrada/salida salvo que quieras forzar READY:
      // resetCycleReady();
    } break;
  }

  estadoPuerta = String(status);
}

// Gestiona rechazo con detalle (r=KO)
static void handleValidationError(int status, const String& ed, const char* from) {
  resetCycleReady();

  // Mensaje de usuario (ajusta ‘ed’ a los literales de tu backend)
  if      (ed == "TICKET_ALREADY_USED")  feedbackError("Entrada ya utilizada");
  else if (ed == "TICKET_NOT_FOUND")     feedbackError("Entrada no encontrada");
  else if (ed == "TICKET_EXPIRED")       feedbackError("Entrada caducada");
  else if (ed == "NOT_TIME_YET")         feedbackError("Fuera de franja horaria");
  else if (ed == "EVENT_CLOSED")         feedbackError("Evento cerrado");
  else if (ed == "DIRECTION_MISMATCH")   feedbackError("Dirección incorrecta");
  else if (ed == "RATE_LIMIT")           feedbackError("Demasiadas lecturas");
  else                                    feedbackError(ed.length() ? ed : "Validación rechazada");

  if (debugSerie) {
    Serial.print("[API]["); Serial.print(from);
    Serial.print("] ERROR status="); Serial.print(status);
    Serial.print(" ed="); Serial.println(ed);
  }
}

// ========================= Serializadores =========================

void serializaInicio() {
  JsonDocument doc;
  doc["r"]       = "OK";               // PRIMERO
  doc["id"]      = DEVICE_ID;
  doc["status"]  = estadoPuerta;
  doc["ec"]      = "CMD_READY";
  doc["version"] = enVersion;

  outputInicio.remove(0);
  serializeJson(doc, outputInicio);
}

void serializaEstado() {
  JsonDocument doc;
  doc["r"]      = "OK";                // PRIMERO
  doc["id"]     = DEVICE_ID;
  doc["status"] = estadoPuerta;
  doc["ec"]     = "CMD_READY";

  outputEstado.remove(0);
  serializeJson(doc, outputEstado);
}

void serializaQR() {
  JsonDocument doc;
  doc["r"]       = "OK";
  doc["id"]      = DEVICE_ID;
  doc["status"]  = estadoPuerta;
  doc["ec"]      = estadoMaquina;
  doc["barcode"] = ultimoTicket;

  outputTicket.remove(0);
  serializeJson(doc, outputTicket);
}

void serializaPaso() {
  JsonDocument doc;
  doc["r"]       = "OK";
  doc["id"]      = DEVICE_ID;
  doc["status"]  = estadoPuerta;
  doc["barcode"] = ultimoPaso;
  doc["ec"]      = "CMD_PASS_OK";
  doc["np"]      = pasoActual;
  doc["nt"]      = pasosTotales;

  outputPaso.remove(0);
  serializeJson(doc, outputPaso);
  ultimoPaso = "";
  if (pasoActual == -1) pasoActual = 0;
}

void serializaReportFailure() {
  JsonDocument doc;
  doc["r"]      = "KO";               // PRIMERO, en fallo
  doc["id"]     = DEVICE_ID;
  doc["status"] = "CMD_FAILURE";
  doc["ec"]     = errorCode;
#if !defined(JSON_MINIMO)
  if (errorCode == "500") { doc["time"] = String(segundosAhora); }
#endif
  outputInicio.remove(0);
  serializeJson(doc, outputInicio);
}

// ========================= Deserializadores =========================

void descifraEstado() {
  JsonDocument doc;
  if (deserializeJson(doc, estadoRecibido)) return;

  if (!jsonRok(doc)) {
    if (debugSerie) Serial.println("[API][estado] r!=OK");
    estadoRecibido = "";
    return;
  }

  String sStat = doc["status"] | "";
  String ec    = doc["ec"]     | "";
  int st       = toIntSafe(sStat, 0);

  if (st) {
    applyStatus(st, ec, /*nt*/0, /*np*/0, "estado");
  } else if (debugSerie) {
    Serial.println("[API][estado] sin status válido");
  }
  estadoRecibido = "";
}

void descifraQR() {
  JsonDocument doc;
  if (deserializeJson(doc, ticketRecibido)) {
    if (debugSerie) Serial.println("[API][qr] JSON inválido");
    g_validateOutcome = VERROR;
    g_lastEd = "JSON_INVALID";
    ticketRecibido = "";
    return;
  }

  const bool rok = jsonRok(doc);
  String sStat = doc["status"] | "";
  String ec    = doc["ec"]     | "";
  String ed    = doc["ed"]     | "";
  String ntS   = doc["nt"]     | "";
  String npS   = doc["np"]     | "";

  int st  = toIntSafe(sStat, 0);
  int nt  = toIntSafe(ntS, 1);
  int np  = toIntSafe(npS, 0);

  if (rok) {
    // r=OK → flujos normales
    if      (st == 201) { g_validateOutcome = VAUTH_IN;  applyStatus(st, ec, nt, np, "qr"); }
    else if (st == 202) { g_validateOutcome = VAUTH_OUT; applyStatus(st, ec, nt, np, "qr"); }
    else                { g_validateOutcome = VNONE;     applyStatus(st, ec, nt, np, "qr"); }
  } else {
    // r=KO → denegado con detalle
    if (st >= 400) {
      g_validateOutcome = VDENIED;
      g_lastEd   = ed;
      g_lastHttp = st;
      handleValidationError(st, ed, "qr");   // resetea a READY
    } else {
      g_validateOutcome = VERROR;
      g_lastEd = ed.length() ? ed : "KO_WITHOUT_STATUS";
    }
  }

  ticketRecibido = "";
}

void descifraPaso() {
  JsonDocument doc;
  if (deserializeJson(doc, pasoRecibido)) {
    if (debugSerie) Serial.println("[API][pass] JSON inválido");
    pasoRecibido = "";
    return;
  }

  if (!jsonRok(doc)) {
    if (debugSerie) Serial.println("[API][pass] r!=OK");
    pasoRecibido = "";
    return;
  }

  String sStat = doc["status"] | "";
  String ec    = doc["ec"]     | "";
  String ntS   = doc["nt"]     | "";
  String npS   = doc["np"]     | "";

  int st  = toIntSafe(sStat, 0);
  int nt  = toIntSafe(ntS, pasosTotales);
  int np  = toIntSafe(npS, contadorPasos);

  if (st) {
    applyStatus(st, ec, nt, np, "pass");
  } else if (debugSerie) {
    Serial.println("[API][pass] sin status válido");
  }

  pasoRecibido = "";
}

