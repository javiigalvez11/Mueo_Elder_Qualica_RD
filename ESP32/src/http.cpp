#include "http.hpp"
#include "definiciones.hpp"
#include "json.hpp" // serializa*/descifra* + globals (output*, *Recibido, flags…)
#include "logBuf.hpp"

#include <Ethernet.h>
#include <Update.h> // OTA en ESP32
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
// ================== Helpers internos (solo en este .cpp) ====================


// Ajusta si quieres truncar para no llenar el log
static const size_t HTTP_LOG_MAX_CHARS = 512;

static String truncateForLog(const String &s, size_t maxChars)
{
  if (maxChars == 0 || s.length() <= maxChars)
    return s;
  return s.substring(0, maxChars) + "...(truncado)";
}
static void log_line_both(const char *fmt, ...)
{
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  logbuf_pushf("%s", buf);
  if (debugSerie)
    Serial.println(buf);
}

// Mete texto largo en logBuf/Serial troceado para que NO se corte
static void log_text_both(const char *prefix, const String &text, size_t chunk = 180)
{
  if (prefix && *prefix)
    log_line_both("%s", prefix);

  for (size_t i = 0; i < text.length(); i += chunk)
  {
    String part = text.substring(i, min(i + chunk, (size_t)text.length()));
    log_line_both("%s", part.c_str());
  }
}

static void dumpJsonFieldsFromString(const char *tag, const String &json)
{
  if (!debugSerie)
    return;

  JsonDocument d;
  DeserializationError err = deserializeJson(d, json);

  Serial.print("[HTTP][");
  Serial.print(tag);
  Serial.println("] Campos:");

  if (err)
  {
    Serial.print("  (JSON inválido) ");
    Serial.println(err.c_str());
    Serial.println("------------------------------");
    Serial.println("");
    return;
  }

  JsonObjectConst obj = d.as<JsonObjectConst>();
  if (obj.isNull())
  {
    Serial.println("  (no es un objeto JSON)");
    Serial.println("------------------------------");
   Serial.println("");
    return;
  }

  for (JsonPairConst kv : obj)
  {
    Serial.print("  - ");
    Serial.print(kv.key().c_str());
    Serial.print(" = ");
    serializeJson(kv.value(), Serial);
    Serial.println();
  }
  Serial.println("------------------------------");
  Serial.println("");
}

template <typename TClient>
static bool readLine(EthernetClient &c, String &line, uint32_t timeout_ms)
{
  line = ""; 
  line.reserve(128); // IMPORTANTE: Reserva espacio para evitar reasignaciones constantes
  
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms)
  {
    while (c.available())
    {
      char ch = (char)c.read();
      if (ch == '\r') continue;
      if (ch == '\n') return true;
      
      // Solo añade si hay espacio razonable, o deja que String crezca en bloques
      line += ch; 
    }
    delay(1);
  }
  return false;
}

static bool parseHttpUrl(const String &url, String &host, uint16_t &port, String &path)
{
  host = "";
  path = "/";
  port = 80;
  String u = url;
  u.trim();
  String rest;

  if (u.startsWith("http://"))
  {
    rest = u.substring(7);
  }
  else if (u.startsWith("https://"))
  {
    rest = u.substring(8);
    port = 443; // por defecto para HTTPS
  }
  else
  {
    return false;
  }
  int slash = rest.indexOf('/');
  String hostport = (slash >= 0) ? rest.substring(0, slash) : rest;
  path = (slash >= 0) ? rest.substring(slash) : "/";
  int colon = hostport.indexOf(':');
  if (colon >= 0)
  {
    host = hostport.substring(0, colon);
    port = (uint16_t)hostport.substring(colon + 1).toInt();
    if (port == 0)
      port = 80;
  }
  else
  {
    host = hostport;
  }
  host.trim();
  path.trim();
  return host.length() > 0;
}

static bool readLine(EthernetClient &c, String &line, uint32_t timeout_ms)
{
  line.remove(0);
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms)
  {
    while (c.available())
    {
      char ch = (char)c.read();
      if (ch == '\r')
        continue;
      if (ch == '\n')
        return true;
      line += ch;
    }
    delay(1);
  }
  return false;
}

// POST application/json → devuelve body en 'response', true si 2xx
static bool postJSON(const String &url, const String &payload, String &response)
{
  response.clear(); // Limpiamos la respuesta anterior

  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
  {
    log_line_both("[HTTP][ETH][ERR] Ethernet sin IP.");
    return false;
  }
  if (!url.startsWith("http://"))
  {
    log_line_both("[HTTP][ETH][ERR] Solo HTTP (no HTTPS) con W5500.");
    return false;
  }

  String host, path;
  uint16_t port;
  if (!parseHttpUrl(url, host, port, path))
  {
    log_line_both("[HTTP][ETH][ERR] URL inválida.");
    return false;
  }

  // ---- LOG OUT ----
  log_line_both("[HTTP][ETH] POST %s", url.c_str());
  if (debugSerie)
  {
    Serial.println("[HTTP][OUT] Payload:");
    Serial.println(truncateForLog(payload, HTTP_LOG_MAX_CHARS));
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie)
  {
    Serial.print("[HTTP][ETH] Conectando ");
    Serial.print(host);
    Serial.print(':');
    Serial.print(port);
    Serial.print(" ... ");
  }
  delay(5);

  if (!client.connect(host.c_str(), port))
  {
    if (debugSerie) Serial.println("FALLO");
    log_line_both("[HTTP][ETH][ERR] connect %s:%u FAILED", host.c_str(), port);
    return false;
  }

  if (debugSerie) Serial.println("OK");
  log_line_both("[HTTP][ETH] Conectado %s:%u", host.c_str(), port);

  // ---- Enviar petición ----
  client.print("POST ");
  client.print(path);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host);
  if (port != 80)
  {
    client.print(':');
    client.print(port);
  }
  client.println();
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.print(payload);

  // ---- Leer status line ----
  String line;
  if (!readLine(client, line, HTTP_TIMEOUT_MS))
  {
    client.stop();
    log_line_both("[HTTP][ETH][ERR] Timeout leyendo status line");
    return false;
  }

  int sp1 = line.indexOf(' ');
  int sp2 = (sp1 >= 0) ? line.indexOf(' ', sp1 + 1) : -1;
  int status = (sp2 > sp1) ? line.substring(sp1 + 1, sp2).toInt() : 0;

  // ---- Leer headers ----
  bool chunked = false;
  size_t contentLen = 0;

  while (readLine(client, line, HTTP_TIMEOUT_MS))
  {
    if (line.length() == 0) break; // Fin de headers

    String l = line;
    l.toLowerCase();
    if (l.startsWith("transfer-encoding:") && l.indexOf("chunked") >= 0)
      chunked = true;
    else if (l.startsWith("content-length:"))
    {
      int colon = line.indexOf(':');
      if (colon >= 0)
        contentLen = line.substring(colon + 1).toInt();
    }
  }

  // ---- Leer body (OPTIMIZADO CON BUFFER) ----
  
  // 1. Reservar memoria total para el String final (si sabemos la longitud)
  if (contentLen > 0) response.reserve(contentLen + 16);
  else response.reserve(512); 

  // 2. Buffer temporal en pila para leer bloques (reduce llamadas a malloc)
  const size_t BUF_SIZE = 128;
  uint8_t buff[BUF_SIZE]; 

  if (chunked)
  {
    for (;;)
    {
      String sz;
      if (!readLine(client, sz, HTTP_TIMEOUT_MS))
      {
        client.stop();
        log_line_both("[HTTP][ETH][ERR] Timeout leyendo chunk size");
        return false;
      }
      sz.trim();
      if (sz.length() == 0) // A veces llega una línea vacía antes del tamaño
      {
        if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
        sz.trim();
      }

      unsigned long n = strtoul(sz.c_str(), nullptr, 16);
      if (n == 0)
      {
        readLine(client, line, HTTP_TIMEOUT_MS); // trailing CRLF final
        break;
      }

      // Leer el chunk de 'n' bytes usando buffer
      uint32_t t0 = millis();
      while (n > 0 && millis() - t0 < HTTP_TIMEOUT_MS)
      {
        if (client.available()) {
          // Leemos el mínimo entre lo que queda (n) y el tamaño del buffer
          int toRead = (n > BUF_SIZE) ? BUF_SIZE : n;
          int leidos = client.read(buff, toRead);
          
          if (leidos > 0) {
             // Volcado rápido al String
             for(int i=0; i<leidos; i++) response += (char)buff[i];
             n -= leidos;
             t0 = millis(); // Reset timeout tras leer datos
          }
        } else {
          delay(1);
        }
      }
      readLine(client, line, HTTP_TIMEOUT_MS); // CRLF del chunk
    }
  }
  else if (contentLen > 0)
  {
    // Lectura estándar con Content-Length
    size_t rem = contentLen;
    uint32_t t0 = millis();
    while (rem > 0 && millis() - t0 < HTTP_TIMEOUT_MS)
    {
      if (client.available()) {
        int toRead = (rem > BUF_SIZE) ? BUF_SIZE : rem;
        int leidos = client.read(buff, toRead);

        if (leidos > 0) {
           for(int i=0; i<leidos; i++) response += (char)buff[i];
           rem -= leidos;
           t0 = millis();
        }
      } else {
        delay(1);
      }
    }
  }
  else
  {
    // Lectura hasta cierre de conexión (sin Content-Length)
    uint32_t t0 = millis();
    while (client.connected() || client.available())
    {
      if (millis() - t0 > HTTP_TIMEOUT_MS) break;

      int avail = client.available();
      if (avail > 0) {
        int toRead = (avail > BUF_SIZE) ? BUF_SIZE : avail;
        int leidos = client.read(buff, toRead);
        
        if (leidos > 0) {
           for(int i=0; i<leidos; i++) response += (char)buff[i];
           t0 = millis();
        }
      } else {
        delay(1);
      }
    }
  }

  client.stop();

  if (debugSerie)
  {
    Serial.println("[HTTP][ETH] Respuesta:");
    Serial.println(truncateForLog(response, HTTP_LOG_MAX_CHARS));
  }

  // Dump de campos recibidos
  dumpJsonFieldsFromString("IN", response);

  return (status >= 200 && status < 300);
}

// POST text/plain → devuelve body en 'response', true si 2xx
static bool postPlain(const String &url, const String &payload, String &response)
{
  response.clear();

  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
  {
    if (debugSerie)
      Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }
  if (!url.startsWith("http://"))
  {
    if (debugSerie)
      Serial.println(F("[HTTP] Solo HTTP (no HTTPS) con W5500."));
    return false;
  }

  String host, path;
  uint16_t port;
  if (!parseHttpUrl(url, host, port, path))
  {
    if (debugSerie)
      Serial.println(F("[HTTP] URL inválida."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie)
  {
    Serial.print(F("[HTTP][ETH] Conectando "));
    Serial.print(host);
    Serial.print(':');
    Serial.print(port);
    Serial.print(F(" ... "));
  }
  if (!client.connect(host.c_str(), port))
  {
    if (debugSerie)
      Serial.println(F("FALLO"));
    return false;
  }
  if (debugSerie)
    Serial.println(F("OK"));

  // --- Enviar petición ---
  client.print(F("POST "));
  client.print(path);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.print(host);
  if (port != 80)
  {
    client.print(':');
    client.print(port);
  }
  client.println();
  client.println(F("Content-Type: text/plain"));
  client.println(F("Connection: close"));
  client.print(F("Content-Length: "));
  client.println(payload.length());
  client.println();
  client.print(payload);

  // --- Leer status line ---
  String line;
  if (!readLine(client, line, HTTP_TIMEOUT_MS))
  {
    client.stop();
    return false;
  }
  int sp1 = line.indexOf(' '), sp2 = (sp1 >= 0) ? line.indexOf(' ', sp1 + 1) : -1;
  int status = (sp2 > sp1) ? line.substring(sp1 + 1, sp2).toInt() : 0;

  // --- Headers/body (idéntico al de JSON) ---
  bool chunked = false;
  size_t contentLen = 0;
  while (readLine(client, line, HTTP_TIMEOUT_MS))
  {
    if (line.length() == 0)
      break;
    String l = line;
    l.toLowerCase();
    if (l.startsWith(F("transfer-encoding:")) && l.indexOf(F("chunked")) >= 0)
      chunked = true;
    else if (l.startsWith(F("content-length:")))
    {
      int colon = line.indexOf(':');
      if (colon >= 0)
        contentLen = line.substring(colon + 1).toInt();
    }
  }

  response.reserve(contentLen ? contentLen : 128);
  if (chunked)
  {
    for (;;)
    {
      String sz;
      if (!readLine(client, sz, HTTP_TIMEOUT_MS))
      {
        client.stop();
        return false;
      }
      sz.trim();
      if (sz.length() == 0)
      {
        if (!readLine(client, sz, HTTP_TIMEOUT_MS))
        {
          client.stop();
          return false;
        }
        sz.trim();
      }
      unsigned long n = strtoul(sz.c_str(), nullptr, 16);
      if (n == 0)
      {
        readLine(client, line, HTTP_TIMEOUT_MS);
        break;
      }
      uint32_t t0 = millis();
      while (n > 0 && millis() - t0 < HTTP_TIMEOUT_MS)
      {
        while (client.available() && n > 0)
        {
          response += (char)client.read();
          n--;
        }
        if (n == 0)
          break;
        delay(1);
      }
      readLine(client, line, HTTP_TIMEOUT_MS);
    }
  }
  else if (contentLen > 0)
  {
    size_t rem = contentLen;
    uint32_t t0 = millis();
    while (rem > 0 && millis() - t0 < HTTP_TIMEOUT_MS)
    {
      while (client.available() && rem > 0)
      {
        response += (char)client.read();
        rem--;
      }
      if (rem == 0)
        break;
      delay(1);
    }
  }
  else
  {
    while (client.connected() || client.available())
    {
      while (client.available())
        response += (char)client.read();
      delay(1);
    }
  }

  client.stop();

  if (debugSerie)
  {

    Serial.println(F("[HTTP][ETH][POST-PLAIN] Respuesta:"));
    Serial.println(response);
  }
  return (status >= 200 && status < 300);
}

// ============= GET crudo (headers + body) para sincronía de hora =============
bool httpGetRaw(const char *host, uint16_t port, const char *path,
                String &headersOut, String &bodyOut)
{
  headersOut = "";
  bodyOut = "";

  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
  {
    if (debugSerie)
      Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie)
  {
    Serial.print(F("[HTTP][ETH] Conectando "));
    Serial.print(host);
    Serial.print(':');
    Serial.print(port);
    Serial.print(path);
    Serial.println(F(" ..."));
  }
  if (!client.connect(host, port))
  {
    if (debugSerie)
      Serial.println(F("[HTTP] Fallo connect()"));
    return false;
  }

  // Petición mínima HTTP/1.1
  client.print(F("GET "));
  client.print(path);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.print(host);
  if (port != 80)
  {
    client.print(':');
    client.print(port);
  }
  client.println();
  client.println(F("Connection: close"));
  client.println();

  // Leer todo con timeout acumulado
  String raw;
  raw.reserve(1024);
  uint32_t t0 = millis();
  while (client.connected() || client.available())
  {
    while (client.available())
    {
      raw += (char)client.read();
      t0 = millis();
      if ((int)raw.length() > HTTP_MAX_BYTES)
        break;
    }
    if ((int)raw.length() > HTTP_MAX_BYTES)
      break;
    if (millis() - t0 > HTTP_READ_TIMEOUT_MS)
      break;
    delay(1);
  }
  client.stop();

  // Separar cabeceras y cuerpo
  int pos = raw.indexOf("\r\n\r\n");
  if (pos < 0)
    pos = raw.indexOf("\n\n");
  if (pos >= 0)
  {
    headersOut = raw.substring(0, pos);
    bodyOut = raw.substring(pos + ((raw[pos] == '\r') ? 4 : 2));
  }
  else
  {
    headersOut = raw;
    bodyOut = "";
  }

  // Comprobar 200 OK
  if (headersOut.startsWith("HTTP/1.1 200") || headersOut.startsWith("HTTP/1.0 200"))
  {
    return true;
  }
  else
  {
    if (debugSerie)
      Serial.println(F("[HTTP] Respuesta no 200"));
    return false;
  }
}

// ============= POST crudo (headers + body) si alguna vez lo necesitas =========
bool httpPostRaw(const char *host, uint16_t port, const char *path,
                 const String &payloadJSON,
                 String &outHeaders, String &outBody)
{
  outHeaders.remove(0);
  outBody.remove(0);

  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
  {
    if (debugSerie)
      Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie)
  {
    Serial.print(F("[HTTP][ETH] Conectando "));
    Serial.print(host);
    Serial.print(':');
    Serial.print(port);
    Serial.print(F(" ... "));
  }
  if (!client.connect(host, port))
  {
    if (debugSerie)
      Serial.println(F("FALLO"));
    return false;
  }
  if (debugSerie)
    Serial.println(F("OK"));

  // ---- Envío petición ----
  client.print(F("POST "));
  client.print(path);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.print(host);
  if (port != 80)
  {
    client.print(':');
    client.print(port);
  }
  client.println();
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print(F("Content-Length: "));
  client.println(payloadJSON.length());
  client.println();
  client.print(payloadJSON);

  // ---- Status line ----
  String line;
  if (!readLine(client, line, HTTP_TIMEOUT_MS))
  {
    client.stop();
    return false;
  }
  int sp1 = line.indexOf(' '), sp2 = (sp1 >= 0) ? line.indexOf(' ', sp1 + 1) : -1;
  int status = (sp2 > sp1) ? line.substring(sp1 + 1, sp2).toInt() : 0;

  // ---- Headers completos ----
  bool chunked = false;
  size_t contentLen = 0;
  for (;;)
  {
    if (!readLine(client, line, HTTP_TIMEOUT_MS))
    {
      client.stop();
      return false;
    }
    if (line.length() == 0)
      break;
    outHeaders += line;
    outHeaders += "\r\n";
    String l = line;
    l.toLowerCase();
    if (l.startsWith(F("transfer-encoding:")) && l.indexOf(F("chunked")) >= 0)
      chunked = true;
    else if (l.startsWith(F("content-length:")))
    {
      int colon = line.indexOf(':');
      if (colon >= 0)
        contentLen = line.substring(colon + 1).toInt();
    }
  }

  // ---- Body ----
  if (chunked)
  {
    for (;;)
    {
      String sz;
      if (!readLine(client, sz, HTTP_TIMEOUT_MS))
      {
        client.stop();
        return false;
      }
      sz.trim();
      if (sz.length() == 0)
      {
        if (!readLine(client, sz, HTTP_TIMEOUT_MS))
        {
          client.stop();
          return false;
        }
        sz.trim();
      }
      unsigned long n = strtoul(sz.c_str(), nullptr, 16);
      if (n == 0)
      {
        readLine(client, line, HTTP_TIMEOUT_MS);
        break;
      }
      uint32_t t0 = millis();
      while (n > 0 && millis() - t0 < HTTP_TIMEOUT_MS)
      {
        while (client.available() && n > 0)
        {
          outBody += (char)client.read();
          n--;
        }
        if (n == 0)
          break;
        delay(1);
      }
      readLine(client, line, HTTP_TIMEOUT_MS); // CRLF
    }
  }
  else if (contentLen > 0)
  {
    size_t rem = contentLen;
    uint32_t t0 = millis();
    while (rem > 0 && millis() - t0 < HTTP_TIMEOUT_MS)
    {
      while (client.available() && rem > 0)
      {
        outBody += (char)client.read();
        rem--;
      }
      if (rem == 0)
        break;
      delay(1);
    }
  }
  else
  {
    while (client.connected() || client.available())
    {
      while (client.available())
        outBody += (char)client.read();
      delay(1);
    }
  }

  client.stop();
  if (debugSerie)
  {

    Serial.println(F("[HTTP][ETH] Headers ↓"));
    Serial.println(outHeaders);
  }
  return (status >= 200 && status < 300);
}

// ====================== API alto nivel (tu contrato) =========================

void getInicio()
{
  logbuf_pushf("[API] getInicio()");

  serializaInicio();
  logbuf_pushf("[API] URL: %s/status", serverURL.c_str());
  logbuf_pushf("[API] Payload: %s", truncateForLog(outputInicio, HTTP_LOG_MAX_CHARS).c_str());
  const String url = serverURL + "/status";

  String resp;
  bool ok = postJSON(url, outputInicio, resp);

  if (!ok)
  {
    logbuf_pushf("[API][ERR] status inicio HTTP FAIL");
    if (debugSerie)
      Serial.println(F("[API] ERROR: status inicio"));
  }

  estadoRecibido = resp;
  descifraEstado();

}

void getEstado()
{
  serializaEstado();
  logbuf_pushf("[API][STATUS][OUT] Payload: %s", truncateForLog(outputEstado, HTTP_LOG_MAX_CHARS).c_str());

  const String url = serverURL + "/status";

  String resp;
  bool ok = postJSON(url, outputEstado, resp);

  if (!ok)
  {
    logbuf_pushf("[API][ERR] status HTTP FAIL");
    if (debugSerie)
      Serial.println(F("[API] ERROR: status"));
  }

  estadoRecibido = resp;
  logbuf_pushf("[API][STATUS][IN] Payload: %s", truncateForLog(estadoRecibido, HTTP_LOG_MAX_CHARS).c_str());
  descifraEstado();

}

void postTicket()
{
  serializaQR();
  logbuf_pushf("[API][QR][OUT] Payload: %s", truncateForLog(outputTicket, HTTP_LOG_MAX_CHARS).c_str());


  uint32_t t0 = millis();

  const String url = serverURL + "/validateQR";
  String resp;
  bool okHttp2xx = postJSON(url, outputTicket, resp);

  if (resp.length() > 0)
  {
    ticketRecibido = resp;
    logbuf_pushf("[API][QR][IN] Payload: %s", truncateForLog(ticketRecibido, HTTP_LOG_MAX_CHARS).c_str());
    descifraQR();
  }
  else
  {
    logbuf_pushf("[API][ERR] validateQR EMPTY_BODY");
    g_validateOutcome = VERROR;
    g_lastEd = "EMPTY_BODY";
    resetCycleReady();
  }
}

void postPaso()
{
  serializaPaso();
  logbuf_pushf("[API][PASS][OUT] Payload: %s", truncateForLog(outputPaso, HTTP_LOG_MAX_CHARS).c_str());

  const String url = serverURL + "/validatePass";

  String resp;
  bool ok = postJSON(url, outputPaso, resp);

  if (!ok)
  {
    logbuf_pushf("[API][ERR] validatePass HTTP FAIL");
    resetCycleReady();
    return;
  }
  
  pasoRecibido = resp;
  logbuf_pushf("[API][PASS][IN] Payload: %s", truncateForLog(pasoRecibido, HTTP_LOG_MAX_CHARS).c_str());
  descifraPaso();
}

void reportFailure()
{
  serializaReportFailure();
  logbuf_pushf("[API][FAIL][OUT] Payload: %s", truncateForLog(outputInicio, HTTP_LOG_MAX_CHARS).c_str());
  const String url = serverURL + "/reportFailure";
  String resp;
  bool ok = postJSON(url, outputInicio /* reutilizo buffer */, resp);
  if (!ok)
  {
  }
  {
    logbuf_pushf("[API][ERR] reportFailure HTTP FAIL");
    if (debugSerie)
      Serial.println(F("[API] ERROR: reportFailure"));
  }
  estadoRecibido = resp;
  logbuf_pushf("[API][FAIL][IN] Payload: %s", truncateForLog(outputInicio, HTTP_LOG_MAX_CHARS).c_str());
  descifraEstado();
}

// ================== ENTRADAS (texto plano) y PENDIENTES ======================

bool getEntradas(String &outTexto)
{
  serializaEstado();
  logbuf_pushf("[API][ENTRIES][OUT] Payload: %s", truncateForLog(outputEstado, HTTP_LOG_MAX_CHARS).c_str());
  const String url = serverURL + "/entries";

  String resp;
  bool ok = postJSON(url, outputEstado, resp);
  if (!ok)
    return false;
  outTexto = resp;
  return true;
}

bool postPendientesBloque(const String &contenido)
{
  const String url = serverURL + "/entries/pending";
  String resp;
  bool ok = postPlain(url, contenido, resp);
  return ok;
}
