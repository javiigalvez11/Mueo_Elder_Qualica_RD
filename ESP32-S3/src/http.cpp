#include "http.hpp"
#include "definiciones.hpp"
#include "json.hpp"
#include "logBuf.hpp"

#include <Ethernet.h>
#include <Update.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ================== Helpers internos ====================

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

// Helper para leer líneas usando la clase base Client (funciona con WiFi y Ethernet)
static bool readLine(Client &c, String &line, uint32_t timeout_ms)
{
  line = "";
  line.reserve(128);
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
    yield(); // Evita bloqueos del WDT
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
    rest = u.substring(7);
  else if (u.startsWith("https://"))
  {
    rest = u.substring(8);
    port = 443;
  }
  else
    return false;

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
    host = hostport;

  host.trim();
  path.trim();
  return host.length() > 0;
}

// --- IMPRESIÓN ATÓMICA DE CAMPOS JSON ---
static void dumpJsonFieldsFromString(const char *tag, const String &json)
{
  if (!debugSerie)
    return;

  JsonDocument d;
  DeserializationError err = deserializeJson(d, json);

  // Construimos TODO el mensaje en memoria primero
  String salida;
  salida.reserve(512);
  salida += "[HTTP][";
  salida += tag;
  salida += "] Campos:\n";

  if (err)
  {
    salida += "  (JSON inválido) ";
    salida += err.c_str();
    salida += "\n";
    Serial.print(salida); // Impresión de un solo golpe
    return;
  }

  JsonObjectConst obj = d.as<JsonObjectConst>();
  if (obj.isNull())
  {
    salida += "  (no es un objeto JSON)\n";
    Serial.print(salida);
    return;
  }

  for (JsonPairConst kv : obj)
  {
    salida += "  - ";
    salida += kv.key().c_str();
    salida += " = ";
    String valStr;
    serializeJson(kv.value(), valStr);
    salida += valStr;
    salida += "\n";
  }

  // Imprimimos todo el bloque construido, sin interrupciones posibles
  Serial.print(salida);
}

// ====================== LÓGICA DE COMUNICACIÓN =========================

static bool postJSON(const String &url, const String &payload, String &response)
{
  response.clear();

  // 1. Validar conexión según conexionRed (0: WiFi, 1: Ethernet)
  if (conexionRed == 0)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      log_line_both("[HTTP][ERR] WiFi no conectado.");
      return false;
    }
  }
  else
  {
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
    {
      log_line_both("[HTTP][ERR] Ethernet sin IP.");
      return false;
    }
  }

  String host, path;
  uint16_t port;
  if (!parseHttpUrl(url, host, port, path))
  {
    log_line_both("[HTTP][ERR] URL inválida.");
    return false;
  }

  log_line_both("[HTTP][%s] POST %s", (conexionRed == 0 ? "WiFi" : "ETH"), url.c_str());

  // 2. Selección de Cliente (Abstracción)
  WiFiClient wfClient;
  EthernetClient ethClient;
  Client *client;

  if (conexionRed == 0)
    client = &wfClient;
  else
    client = &ethClient;

  client->setTimeout(HTTP_TIMEOUT_MS / 1000); // Segundos en algunas implementaciones

  if (!client->connect(host.c_str(), port))
  {
    log_line_both("[HTTP][ERR] connect %s:%u FAILED", host.c_str(), port);
    return false;
  }

  // 3. Enviar HTTP Request
  client->print("POST ");
  client->print(path);
  client->println(" HTTP/1.1");
  client->print("Host: ");
  client->print(host);
  if (port != 80)
  {
    client->print(':');
    client->print(port);
  }
  client->println();
  client->println("Content-Type: application/json");
  client->println("Connection: close");
  client->print("Content-Length: ");
  client->println(payload.length());
  client->println();
  client->print(payload);

  dumpJsonFieldsFromString("OUT", payload);

  // 4. Leer Status Line
  String line;
  if (!readLine(*client, line, HTTP_TIMEOUT_MS))
  {
    client->stop();
    log_line_both("[HTTP][ERR] Timeout status line");
    return false;
  }

  int sp1 = line.indexOf(' ');
  int sp2 = (sp1 >= 0) ? line.indexOf(' ', sp1 + 1) : -1;
  int status = (sp2 > sp1) ? line.substring(sp1 + 1, sp2).toInt() : 0;

  // 5. Leer Headers
  bool chunked = false;
  size_t contentLen = 0;
  while (readLine(*client, line, HTTP_TIMEOUT_MS) && line.length() > 0)
  {
    line.toLowerCase();
    if (line.startsWith("transfer-encoding:") && line.indexOf("chunked") >= 0)
      chunked = true;
    else if (line.startsWith("content-length:"))
    {
      contentLen = line.substring(line.indexOf(':') + 1).toInt();
    }
  }

  // 6. Leer Body con Buffer
  if (contentLen > 0)
    response.reserve(contentLen + 1);
  const size_t BUF_SIZE = 128;
  uint8_t buff[BUF_SIZE];

  if (chunked)
  {
    while (true)
    {
      String szLine;
      if (!readLine(*client, szLine, HTTP_TIMEOUT_MS))
        break;
      long chunkSize = strtol(szLine.c_str(), NULL, 16);
      if (chunkSize == 0)
        break;

      while (chunkSize > 0)
      {
        size_t toRead = min((size_t)chunkSize, BUF_SIZE);
        int n = client->read(buff, toRead);
        if (n > 0)
        {
          for (int i = 0; i < n; i++)
            response += (char)buff[i];
          chunkSize -= n;
        }
      }
      readLine(*client, szLine, HTTP_TIMEOUT_MS); // Consumir CRLF
    }
  }
  else
  {
    uint32_t tStart = millis();
    while ((client->connected() || client->available()) && (millis() - tStart < HTTP_TIMEOUT_MS))
    {
      if (client->available())
      {
        int n = client->read(buff, BUF_SIZE);
        if (n > 0)
        {
          for (int i = 0; i < n; i++)
            response += (char)buff[i];
          if (contentLen > 0 && response.length() >= contentLen)
            break;
        }
      }
      else
        delay(1);
    }
  }

  client->stop();
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

  // --- CONEXIÓN Y LOG ATÓMICO ---
  bool connected = client.connect(host.c_str(), port);
  if (debugSerie)
  {
    Serial.printf("[HTTP][ETH] Conectando %s:%u ... %s\n", host.c_str(), port, connected ? "OK" : "FALLO");
  }
  if (!connected)
    return false;

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

  // --- Headers/body ---
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

  // --- IMPRESIÓN ATÓMICA DE RESPUESTA ---
  if (debugSerie)
  {
    Serial.printf("[HTTP][ETH][POST-PLAIN] Respuesta:\n%s\n", response.c_str());
  }
  return (status >= 200 && status < 300);
}

// ====================== API ALTO NIVEL =========================

void getInicio()
{
  serializaInicio();
  logbuf_pushf("[API][INICIO][OUT] Payload: %s", truncateForLog(outputInicio, HTTP_LOG_MAX_CHARS).c_str());

  String resp;
  if (postJSON(serverURL + "/inicio", outputInicio, resp))
  {
    estadoRecibido = resp;
    logbuf_pushf("[API][INICIO][IN] Payload: %s", truncateForLog(estadoRecibido, HTTP_LOG_MAX_CHARS).c_str());
    descifraEstado();
    iniciOk = true;
  }
  else
  {
    log_line_both("[API][ERR] getInicio FAIL");
    iniciOk = false;
  }
}

void getEstado()
{
  serializaEstado();
  logbuf_pushf("[API][STATUS][OUT] Payload: %s", truncateForLog(outputEstado, HTTP_LOG_MAX_CHARS).c_str());

  String resp;
  if (postJSON(serverURL + "/status", outputEstado, resp))
  {
    estadoRecibido = resp;
    logbuf_pushf("[API][STATUS][IN] Payload: %s", truncateForLog(estadoRecibido, HTTP_LOG_MAX_CHARS).c_str());

    descifraEstado();
  }
  else
  {
    log_line_both("[API][ERR] getEstado FAIL");
  }
}

void postTicket()
{
  serializaQR();
  logbuf_pushf("[API][QR][OUT] Payload: %s", truncateForLog(outputTicket, HTTP_LOG_MAX_CHARS).c_str());

  String resp;
  if (postJSON(serverURL + "/validateQR", outputTicket, resp))
  {
    ticketRecibido = resp;
    logbuf_pushf("[API][QR][IN] Payload: %s", truncateForLog(ticketRecibido, HTTP_LOG_MAX_CHARS).c_str());
    descifraQR();
  }
  else
  {
    log_line_both("[API][ERR] postTicket FAIL");
    g_validateOutcome = VERROR;
    resetCycleReady();
  }
}

void postPaso()
{
  serializaPaso();
  logbuf_pushf("[API][PASS][OUT] Payload: %s", truncateForLog(outputPaso, HTTP_LOG_MAX_CHARS).c_str());

  String resp;
  if (postJSON(serverURL + "/validatePass", outputPaso, resp))
  {
    pasoRecibido = resp;
    logbuf_pushf("[API][PASS][IN] Payload: %s", truncateForLog(pasoRecibido, HTTP_LOG_MAX_CHARS).c_str());
    descifraPaso();
  }
  else
    resetCycleReady();
}

void reportFailure()
{
  serializaReportFailure();
  logbuf_pushf("[API][FAIL][OUT] Payload: %s", truncateForLog(outputReportFailure, HTTP_LOG_MAX_CHARS).c_str());

  String resp;
  if (postJSON(serverURL + "/reportFailure", outputReportFailure, resp))
  {
    estadoRecibido = resp;
    logbuf_pushf("[API][FAIL][IN] Payload: %s", truncateForLog(estadoRecibido, HTTP_LOG_MAX_CHARS).c_str());
    descifraEstado();
  }
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

// ====================== ESTADO RED =========================

bool linkUp()
{
  if (conexionRed == 0)
    return WiFi.status() == WL_CONNECTED;
  return Ethernet.linkStatus() == LinkON;
}

bool netOk()
{
  if (conexionRed == 0)
    return (WiFi.status() == WL_CONNECTED) && (WiFi.localIP() != IPAddress(0, 0, 0, 0));
  return (Ethernet.linkStatus() == LinkON) && (Ethernet.localIP() != IPAddress(0, 0, 0, 0));
}