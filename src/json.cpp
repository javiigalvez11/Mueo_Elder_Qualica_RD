#include "json.hpp"
#include "definiciones.hpp"
#include "rele.hpp" // rele_open / rele_close

// ========================= Helpers internos =========================

static inline int toIntSafe(const String &s, int dflt)
{
  if (s.length() == 0)
    return dflt;
  char *end = nullptr;
  long v = strtol(s.c_str(), &end, 10);
  return (end && *end == '\0') ? (int)v : dflt;
}

// Normaliza 'r' leyendo "r" o "R" (tolerante a mayúsculas/minúsculas y espacios)
static bool jsonRok(const JsonDocument &doc)
{
  String r;
  if (!doc["r"].isNull())
    r = doc["r"].as<String>();
  else if (!doc["R"].isNull())
    r = doc["R"].as<String>();
  else
    r = "";
  r.trim();
  r.toLowerCase();
  return r == "ok";
}

// Versión “tolerante” usada en nuevos deserializadores
static inline bool json_is_ok(const JsonDocument &d)
{
  String r = d["r"] | "";
  r.trim();
  r.toLowerCase();
  return (r == "ok" || r == "success");
}

// Normaliza status que puede venir "201" (string) o 201 (int)
static inline int get_status_as_int(const JsonDocument &d)
{
  if (d["status"].is<int>())
    return (int)d["status"];
  String s = d["status"] | "";
  s.trim();
  return toIntSafe(s, 0);
}

// Construye un “detalle” de error combinando ec + reason si existen
static inline String compose_error_detail(const JsonDocument &d)
{
  String ec = d["ec"] | "";
  String rs = d["reason"] | "";
  if (ec.length() && rs.length())
    return ec + ":" + rs; // p.ej. "ERROR_CONFLICT:too_early"
  if (ec.length())
    return ec;
  if (rs.length())
    return rs;
  String ed = d["ed"] | ""; // compat con formato mock antiguo
  if (ed.length())
    return ed;
  return "UNKNOWN_ERROR";
}

// Extrae un parámetro de 6 dígitos de una cadena tipo query string
static String getSixDigitsParam(const String &s, const char *key)
{
  String pattern = String(key) + "=";
  int p = s.indexOf(pattern);
  if (p < 0)
    return "";
  p += pattern.length();

  int q = s.indexOf('&', p);
  String raw = (q >= 0) ? s.substring(p, q) : s.substring(p);

  // Quedarnos solo con dígitos y tomar los 6 primeros
  String digits;
  digits.reserve(6);
  for (int i = 0; i < raw.length() && digits.length() < 6; ++i)
  {
    char c = raw.charAt(i);
    if (c >= '0' && c <= '9')
      digits += c;
  }
  return digits; // "" si no encontró 6 dígitos
}

// Acciones visuales/sonoras opcionales
static inline void feedbackError(const String &ed)
{
  if (debugSerie)
  {
    Serial.print("[IO][ERROR] ");
    Serial.println(ed);
  }
}

// Pequeño helper para toggle de apertura continua
static void aplicarAperturaContinua(bool on)
{
  int nuevo = on ? 1 : 0;
  if (abreContinua == nuevo)
    return;
  abreContinua = nuevo;
  if (abreContinua)
  {
    if (debugSerie)
      Serial.println("[IO] Apertura continua: ON");
    rele_open();
  }
  else
  {
    if (debugSerie)
      Serial.println("[IO] Apertura continua: OFF");
    rele_close();
  }
}

// Resetea el ciclo a READY con puerta 200
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

static void debugDumpJson(const char *tag, const JsonDocument &doc)
{
  if (!debugSerie)
    return;

  Serial.print("[HTTP][OUT] ");
  Serial.print(tag);
  Serial.println(" → JSON:");
  serializeJson(doc, Serial);
  Serial.println();
  Serial.println("[HTTP][OUT] Campos:");

  // Clave: iterar como objeto *const*
  JsonObjectConst obj = doc.as<JsonObjectConst>();
  if (!obj.isNull())
  {
    for (JsonPairConst kv : obj)
    {
      Serial.print("  - ");
      Serial.print(kv.key().c_str());
      Serial.print(" = ");
      // Imprime el valor sea del tipo que sea
      serializeJson(kv.value(), Serial);
      Serial.println();
    }
  }
  else
  {
    Serial.println("  (no es un objeto JSON)");
  }

  Serial.println("------------------------------");
}

// Función helper para serializar el enum como string
static inline const char *ec_to_str(CmdType c)
{
  switch (c)
  {
  case CMD_READY:
    return "CMD_READY";
  case CMD_OPEN_CONTINUOUS:
    return "CMD_OPEN_CONTINUOUS";
  case CMD_VALIDATE_IN:
    return "CMD_VALIDATE_IN";
  case CMD_VALIDATE_OUT:
    return "CMD_VALIDATE_OUT";
  case CMD_PASS_OK:
    return "CMD_PASS_OK";
  case CMD_PASS_TIMEOUT:
    return "CMD_PASS_TIMEOUT";
  case CMD_PASSED_IN:
    return "CMD_PASSED_IN";
  case CMD_PASSED_OUT:
    return "CMD_PASSED_OUT";
  case CMD_RESTART:
    return "CMD_RESTART";
  case CMD_UPDATE:
    return "CMD_UPDATE";
  default:
    return "CMD_NONE";
  }
}

// Convierte el string 'ec' del backend en el enum CmdType
static inline CmdType cmd_from_ec_string(const String &ec)
{
  if (ec == "CMD_READY")
    return CMD_READY;
  if (ec == "CMD_OPEN_CONTINUOUS")
    return CMD_OPEN_CONTINUOUS;
  if (ec == "CMD_VALIDATE_IN")
    return CMD_VALIDATE_IN;
  if (ec == "CMD_VALIDATE_OUT")
    return CMD_VALIDATE_OUT;
  if (ec == "CMD_PASS_OK")
    return CMD_PASS_OK;
  if (ec == "CMD_PASS_TIMEOUT")
    return CMD_PASS_TIMEOUT;
  if (ec == "CMD_PASSED_IN")
    return CMD_PASSED_IN;
  if (ec == "CMD_PASSED_OUT")
    return CMD_PASSED_OUT;
  if (ec == "CMD_RESTART")
    return CMD_RESTART;
  if (ec == "CMD_UPDATE")
    return CMD_UPDATE;

  // Desconocido o vacío
  return CMD_NONE;
}

// Maneja cualquier estado de puerta de forma centralizada.
// Maneja cualquier estado de puerta de forma centralizada.
static void applyStatus(int status, const String &ec, int nt, int np, const char *from)
{
  // 1) Calcular el nuevo estado de máquina a partir de 'ec' o, si no hay, del 'status'
  CmdType nuevoEstado = cmd_from_ec_string(ec);

  if (nuevoEstado == CMD_NONE)
  {
    // Fallback según el status si el backend no manda 'ec' o manda algo raro
    switch (status)
    {
    case 200:
      nuevoEstado = CMD_READY;
      break;
    case 201:
      nuevoEstado = CMD_VALIDATE_IN;
      break;
    case 202:
      nuevoEstado = CMD_VALIDATE_OUT;
      break;
    case 203:
      nuevoEstado = CMD_OPEN_CONTINUOUS;
      break;
    case 305:
      nuevoEstado = CMD_RESTART;
      break;
    case 310:
      nuevoEstado = CMD_UPDATE;
      break;
    default: /* nos quedamos con CMD_NONE */
      break;
    }
  }

  // 2) Estado de puerta SIEMPRE es el status recibido
  estadoPuerta = String(status);
  // 3) Estado de máquina SIEMPRE es el enum calculado
  estadoMaquina = nuevoEstado;

  // 4) Lógica específica por status (entradas, salidas, etc.)
  switch (status)
  {
  case 200:
  { // READY / heartbeat normal
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.println("] 200 READY");
      if (ec.length())
      {
        Serial.print("  ec=");
        Serial.println(ec);
      }
    }
  }
  break;

  case 201:
  { // ENTRADA autorizada
    activaConecta = 0;
    pasosTotales = (nt > 0) ? nt : 1;
    contadorPasos = (np >= 0) ? np : 0;
    activaEntrada = 1;
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] 201 ENTRADA: nt=");
      Serial.print(pasosTotales);
      Serial.print(" np=");
      Serial.print(contadorPasos);
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
  }
  break;

  case 202:
  { // SALIDA autorizada
    activaConecta = 0;
    pasosTotales = (nt > 0) ? nt : 1;
    contadorPasos = (np >= 0) ? np : 0;
    activaSalida = 1;
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] 202 SALIDA: nt=");
      Serial.print(pasosTotales);
      Serial.print(" np=");
      Serial.print(contadorPasos);
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
  }
  break;

  case 203:
  { // APERTURA CONTINUA (mantener abierta)
    aplicarAperturaContinua(true);
    activaConecta = 1;
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] 203 APERTURA CONTINUA");
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
  }
  break;

  case 300:
  { // CANCELAR/ABORTAR paso
    abortarPaso = 1;
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] 300 CANCELAR");
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
  }
  break;

  case 305:
  { // REINICIAR dispositivo
    resetCycleReady();
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] 305 REINICIAR");
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
    delay(100);
    esp_restart();
  }
  break;

  case 310:
  { // UPDATE OTA
    actualizarFlag = 1;
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] 310 UPDATE");
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
  }
  break;

  default:
  { // 4xx/5xx u otros
    if (debugSerie)
    {
      Serial.print("[API][");
      Serial.print(from);
      Serial.print("] status=");
      Serial.print(status);
      if (ec.length())
      {
        Serial.print(" ec=");
        Serial.print(ec);
      }
      Serial.println();
    }
    // Si quisieras volver siempre a READY en cualquier no-2xx:
    // resetCycleReady();
  }
  break;
  }
}

// Gestiona rechazo con detalle (r=KO / r=error)
static void handleValidationError(int status, const String &ed, const char *from)
{
  resetCycleReady();

  // Mensaje de usuario (ajusta a los literales de tu backend)
  if (ed == "TICKET_ALREADY_USED")
    feedbackError("Entrada ya utilizada");
  else if (ed == "TICKET_NOT_FOUND")
    feedbackError("Entrada no encontrada");
  else if (ed == "TICKET_EXPIRED")
    feedbackError("Entrada caducada");
  else if (ed == "NOT_TIME_YET")
    feedbackError("Fuera de franja horaria");
  else if (ed == "EVENT_CLOSED")
    feedbackError("Evento cerrado");
  else if (ed == "DIRECTION_MISMATCH")
    feedbackError("Dirección incorrecta");
  else if (ed.startsWith("ERROR_CONFLICT:too_early"))
    feedbackError("Demasiado pronto");
  else if (ed.startsWith("ERROR_CONFLICT:too_late"))
    feedbackError("Demasiado tarde");
  else if (ed.startsWith("ERROR_CONFLICT:already_exited"))
    feedbackError("Ya salió previamente");
  else
    feedbackError(ed.length() ? ed : "Validación rechazada");

  if (debugSerie)
  {
    Serial.print("[API][");
    Serial.print(from);
    Serial.print("] ERROR status=");
    Serial.print(status);
    Serial.print(" ed=");
    Serial.println(ed);
  }
}

// ========================= Serializadores =========================

void serializaInicio()
{
  JsonDocument doc;
  doc["r"] = "OK"; // PRIMERO
  doc["id"] = DEVICE_ID;
  // doc["status"]  = estadoPuerta;
  // doc["ec"]      = ec_to_str(estadoMaquina);
  // doc["version"] = enVersion;

  outputInicio.remove(0);
  serializeJson(doc, outputInicio);

  // DEBUG: mostrar lo que enviamos
  debugDumpJson("inicio", doc);
}

void serializaEstado()
{
  JsonDocument doc;
  doc["r"] = "OK"; // PRIMERO
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;
  doc["ec"] = ec_to_str(estadoMaquina);

  outputEstado.remove(0);
  serializeJson(doc, outputEstado);

  // DEBUG: mostrar lo que enviamos
  debugDumpJson("estado", doc);
}

void serializaQR()
{
  JsonDocument doc;
  doc["r"] = "OK";
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;
  doc["ec"] = ec_to_str(estadoMaquina);

  if (ultimoTicket.startsWith("ticket_id="))
  {
    String tid = getSixDigitsParam(ultimoTicket, "ticket_id");
    String eid = getSixDigitsParam(ultimoTicket, "event_id");
    if (tid.length() == 6 && eid.length() == 6)
    {
      doc["ticket_id"] = tid;
      doc["event_id"] = eid;
    }
    else
    {
      // Fallback: enviar crudo si no se pudo parsear
      doc["barcode"] = ultimoTicket;
    }
  }
  else
  {
    doc["barcode"] = ultimoTicket;
  }

  outputTicket.remove(0);
  serializeJson(doc, outputTicket);

  // DEBUG: mostrar lo que enviamos
  debugDumpJson("qr", doc);
}

void serializaPaso()
{
  JsonDocument doc;
  doc["r"] = "OK";
  doc["id"] = DEVICE_ID;
  doc["status"] = estadoPuerta;

  if (ultimoPaso.startsWith("ticket_id="))
  {
    String tid = getSixDigitsParam(ultimoPaso, "ticket_id");
    String eid = getSixDigitsParam(ultimoPaso, "event_id");
    if (tid.length() == 6 && eid.length() == 6)
    {
      doc["ticket_id"] = tid;
      doc["event_id"] = eid;
    }
    else
    {
      // Fallback: enviar crudo si no se pudo parsear
      doc["barcode"] = ultimoPaso; // corregido
    }
  }
  else
  {
    doc["barcode"] = ultimoPaso; // corregido
  }

  doc["ec"] = ec_to_str(estadoMaquina);
  doc["np"] = pasoActual;
  doc["nt"] = pasosTotales;

  outputPaso.remove(0);
  serializeJson(doc, outputPaso);
  if (pasoActual == -1)
    pasoActual = 0;
  ultimoPaso = "";

  // DEBUG: mostrar lo que enviamos
  debugDumpJson("pass", doc);
}

void serializaReportFailure()
{
  JsonDocument doc;
  doc["r"] = "KO";
  doc["id"] = DEVICE_ID;
  doc["status"] = 500;   // numérico
  doc["ec"] = errorCode; // detalle

#if !defined(JSON_MINIMO)
  // Campo extra opcional de diagnóstico
  doc["time"] = String(segundosAhora);
#endif

  outputInicio.remove(0);
  serializeJson(doc, outputInicio);

  // DEBUG: mostrar lo que enviamos
  debugDumpJson("reportFailure", doc);
}

// ========================= Deserializadores =========================

void descifraEstado()
{
  JsonDocument doc;
  if (deserializeJson(doc, estadoRecibido))
    return;

  const bool rok = json_is_ok(doc);
  const int st = get_status_as_int(doc);
  String ec = doc["ec"] | "";

  if (rok)
  {
    if (st)
      applyStatus(st, ec, 0, 0, "estado");
    else if (debugSerie)
      Serial.println("[API][estado] sin status válido");
  }
  else
  {
    // r="error" → manejar 4xx/5xx
    const String detail = compose_error_detail(doc);
    if (st >= 400)
      handleValidationError(st, detail, "estado");
    else
      handleValidationError(500, detail.length() ? detail : "KO_WITHOUT_STATUS", "estado");
  }
  estadoRecibido = "";
}

void descifraQR()
{
  JsonDocument doc;
  if (deserializeJson(doc, ticketRecibido))
  {
    if (debugSerie)
      Serial.println("[API][qr] JSON inválido");
    g_validateOutcome = VERROR;
    g_lastEd = "JSON_INVALID";
    ticketRecibido = "";
    return;
  }

  // --- Campos (robustos a claves ausentes) ---
  const bool rok = json_is_ok(doc);      // nuevo/antiguo
  const int st = get_status_as_int(doc); // "201" o 201 → 201
  String ec = doc["ec"] | "";            // nuevo error code
  String ntS = doc["nt"] | "";           // opcional (mock antiguo)
  String npS = doc["np"] | "";           // opcional (mock antiguo)
  int nt = toIntSafe(ntS, 1);
  int np = toIntSafe(npS, 0);

  // --- ÉXITO (r = ok) ---
  if (rok)
  {
    if (st == 201)
    {
      g_validateOutcome = VAUTH_IN;
      applyStatus(st, ec, nt, np, "qr");
    }
    else if (st == 202)
    {
      g_validateOutcome = VAUTH_OUT;
      applyStatus(st, ec, nt, np, "qr");
    }
    else
    { // 200, 203… u otros definidos por backend
      g_validateOutcome = VNONE;
      applyStatus(st, ec, nt, np, "qr");
    }
    ticketRecibido = "";
    return;
  }

  // --- ERROR (r = error / ko) ---
  const String detail = compose_error_detail(doc);

  if (st >= 500)
  {
    g_validateOutcome = VERROR;
    g_lastEd = detail; // "ERROR_INTERNAL" o "ERROR_CONFLICT:too_early"
    g_lastHttp = st;   // 500, 502…
    handleValidationError(st, g_lastEd, "qr");
  }
  else if (st >= 400)
  {
    g_validateOutcome = VDENIED;
    g_lastEd = detail; // "ERROR_BAD_REQUEST" o "ERROR_CONFLICT:already_exited"
    g_lastHttp = st;   // 400, 401, 403, 404, 409…
    handleValidationError(st, g_lastEd, "qr");
  }
  else
  {
    g_validateOutcome = VERROR;
    g_lastEd = detail.length() ? detail : "KO_WITHOUT_STATUS";
    g_lastHttp = st; // 0
    handleValidationError(500, g_lastEd, "qr");
  }

  ticketRecibido = "";
}

void descifraPaso()
{
  JsonDocument doc;
  if (deserializeJson(doc, pasoRecibido))
  {
    if (debugSerie)
      Serial.println("[API][pass] JSON inválido");
    pasoRecibido = "";
    return;
  }

  const bool rok = json_is_ok(doc);
  const int st = get_status_as_int(doc);
  String ec = doc["ec"] | "";
  String ntS = doc["nt"] | "";
  String npS = doc["np"] | "";
  int nt = toIntSafe(ntS, pasosTotales);
  int np = toIntSafe(npS, contadorPasos);

  if (rok)
  {
    if (st)
      applyStatus(st, ec, nt, np, "pass");
    else if (debugSerie)
      Serial.println("[API][pass] sin status válido");
    pasoRecibido = "";
    return;
  }

  // r="error" → nuevo contrato (4xx/5xx)
  const String detail = compose_error_detail(doc);
  if (st >= 500)
  {
    handleValidationError(st, detail, "pass"); // VERROR semántico
  }
  else if (st >= 400)
  {
    handleValidationError(st, detail, "pass"); // VDENIED
  }
  else
  {
    handleValidationError(500, detail.length() ? detail : "KO_WITHOUT_STATUS", "pass");
  }
  pasoRecibido = "";
}
