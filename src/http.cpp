#include "http.hpp"

#include <Ethernet.h>
#include <Update.h>          // OTA en ESP32
#include "config.hpp"
#include "json.hpp"          // serializa*/descifra* + globals (output*, *Recibido, flags…)

// ================== Helpers internos (solo en este .cpp) ====================

static bool parseHttpUrl(const String& url, String& host, uint16_t& port, String& path) {
  host = ""; path = "/"; port = 80;
  String u = url; u.trim();
  if (!u.startsWith("http://")) return false;   // HTTPS no soportado con W5500
  String rest = u.substring(7);
  int slash = rest.indexOf('/');
  String hostport = (slash >= 0) ? rest.substring(0, slash) : rest;
  path = (slash >= 0) ? rest.substring(slash) : "/";
  int colon = hostport.indexOf(':');
  if (colon >= 0) {
    host = hostport.substring(0, colon);
    port = (uint16_t)hostport.substring(colon + 1).toInt();
    if (port == 0) port = 80;
  } else {
    host = hostport;
  }
  host.trim(); path.trim();
  return host.length() > 0;
}

static bool readLine(EthernetClient& c, String& line, uint32_t timeout_ms) {
  line.remove(0);
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms) {
    while (c.available()) {
      char ch = (char)c.read();
      if (ch == '\r') continue;
      if (ch == '\n') return true;
      line += ch;
    }
    delay(1);
  }
  return false;
}

// POST application/json → devuelve body en 'response', true si 2xx
static bool postJSON(const String& url, const String& payload, String& response) {
  response.clear();

  if (Ethernet.localIP() == IPAddress(0,0,0,0)) {
    if (debugSerie) Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }
  if (!url.startsWith("http://")) {
    if (debugSerie) Serial.println(F("[HTTP] Solo HTTP (no HTTPS) con W5500."));
    return false;
  }

  String host, path; uint16_t port;
  if (!parseHttpUrl(url, host, port, path)) {
    if (debugSerie) Serial.println(F("[HTTP] URL inválida."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie) {
    Serial.print(F("[HTTP][ETH] Conectando ")); Serial.print(host); Serial.print(':'); Serial.print(port); Serial.print(F(" ... "));
  }
  if (!client.connect(host.c_str(), port)) {
    if (debugSerie) Serial.println(F("FALLO"));
    return false;
  }
  if (debugSerie) Serial.println(F("OK"));

  // --- Enviar petición ---
  client.print(F("POST ")); client.print(path); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.print(host); if (port != 80) { client.print(':'); client.print(port); } client.println();
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print  (F("Content-Length: ")); client.println(payload.length());
  client.println();
  client.print(payload);

  // --- Leer status line ---
  String line;
  if (!readLine(client, line, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
  int sp1 = line.indexOf(' '), sp2 = (sp1>=0)? line.indexOf(' ', sp1+1) : -1;
  int status = (sp2>sp1)? line.substring(sp1+1, sp2).toInt() : 0;

  // --- Leer headers ---
  bool chunked = false;
  size_t contentLen = 0;
  while (readLine(client, line, HTTP_TIMEOUT_MS)) {
    if (line.length() == 0) break; // fin headers
    String l = line; l.toLowerCase();
    if (l.startsWith(F("transfer-encoding:")) && l.indexOf(F("chunked")) >= 0) chunked = true;
    else if (l.startsWith(F("content-length:"))) {
      int colon = line.indexOf(':');
      if (colon >= 0) contentLen = line.substring(colon+1).toInt();
    }
  }

  // --- Leer cuerpo ---
  response.reserve(contentLen ? contentLen : 128);
  if (chunked) {
    for (;;) {
      String sz;
      if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
      sz.trim();
      if (sz.length() == 0) { if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; } sz.trim(); }
      unsigned long n = strtoul(sz.c_str(), nullptr, 16);
      if (n == 0) { readLine(client, line, HTTP_TIMEOUT_MS); break; }
      uint32_t t0 = millis();
      while (n > 0 && millis() - t0 < HTTP_TIMEOUT_MS) {
        while (client.available() && n > 0) { response += (char)client.read(); n--; }
        if (n == 0) break;
        delay(1);
      }
      readLine(client, line, HTTP_TIMEOUT_MS); // CRLF de chunk
    }
  } else if (contentLen > 0) {
    size_t rem = contentLen;
    uint32_t t0 = millis();
    while (rem > 0 && millis() - t0 < HTTP_TIMEOUT_MS) {
      while (client.available() && rem > 0) { response += (char)client.read(); rem--; }
      if (rem == 0) break;
      delay(1);
    }
  } else {
    while (client.connected() || client.available()) {
      while (client.available()) response += (char)client.read();
      delay(1);
    }
  }

  client.stop();

  if (debugSerie) {
    Serial.print(F("[HTTP][ETH] Código: ")); Serial.println(status);
    Serial.println(F("[HTTP][ETH] Respuesta:"));
    Serial.println(response);
  }
  return (status >= 200 && status < 300);
}

// POST text/plain → devuelve body en 'response', true si 2xx
static bool postPlain(const String& url, const String& payload, String& response) {
  response.clear();

  if (Ethernet.localIP() == IPAddress(0,0,0,0)) {
    if (debugSerie) Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }
  if (!url.startsWith("http://")) {
    if (debugSerie) Serial.println(F("[HTTP] Solo HTTP (no HTTPS) con W5500."));
    return false;
  }

  String host, path; uint16_t port;
  if (!parseHttpUrl(url, host, port, path)) {
    if (debugSerie) Serial.println(F("[HTTP] URL inválida."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie) {
    Serial.print(F("[HTTP][ETH] Conectando ")); Serial.print(host); Serial.print(':'); Serial.print(port); Serial.print(F(" ... "));
  }
  if (!client.connect(host.c_str(), port)) {
    if (debugSerie) Serial.println(F("FALLO"));
    return false;
  }
  if (debugSerie) Serial.println(F("OK"));

  // --- Enviar petición ---
  client.print(F("POST ")); client.print(path); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.print(host); if (port != 80) { client.print(':'); client.print(port); } client.println();
  client.println(F("Content-Type: text/plain"));
  client.println(F("Connection: close"));
  client.print  (F("Content-Length: ")); client.println(payload.length());
  client.println();
  client.print(payload);

  // --- Leer status line ---
  String line;
  if (!readLine(client, line, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
  int sp1 = line.indexOf(' '), sp2 = (sp1>=0)? line.indexOf(' ', sp1+1) : -1;
  int status = (sp2>sp1)? line.substring(sp1+1, sp2).toInt() : 0;

  // --- Headers/body (idéntico al de JSON) ---
  bool chunked = false; size_t contentLen = 0;
  while (readLine(client, line, HTTP_TIMEOUT_MS)) {
    if (line.length() == 0) break;
    String l = line; l.toLowerCase();
    if (l.startsWith(F("transfer-encoding:")) && l.indexOf(F("chunked")) >= 0) chunked = true;
    else if (l.startsWith(F("content-length:"))) {
      int colon = line.indexOf(':');
      if (colon >= 0) contentLen = line.substring(colon+1).toInt();
    }
  }

  response.reserve(contentLen ? contentLen : 128);
  if (chunked) {
    for (;;) {
      String sz;
      if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
      sz.trim(); if (sz.length()==0) { if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; } sz.trim(); }
      unsigned long n = strtoul(sz.c_str(), nullptr, 16);
      if (n == 0) { readLine(client, line, HTTP_TIMEOUT_MS); break; }
      uint32_t t0 = millis();
      while (n > 0 && millis()-t0 < HTTP_TIMEOUT_MS) {
        while (client.available() && n > 0) { response += (char)client.read(); n--; }
        if (n == 0) break;
        delay(1);
      }
      readLine(client, line, HTTP_TIMEOUT_MS);
    }
  } else if (contentLen > 0) {
    size_t rem = contentLen; uint32_t t0 = millis();
    while (rem > 0 && millis()-t0 < HTTP_TIMEOUT_MS) {
      while (client.available() && rem > 0) { response += (char)client.read(); rem--; }
      if (rem == 0) break;
      delay(1);
    }
  } else {
    while (client.connected() || client.available()) {
      while (client.available()) response += (char)client.read();
      delay(1);
    }
  }

  client.stop();

  if (debugSerie) {
    Serial.print(F("[HTTP][ETH][POST-PLAIN] Código: ")); Serial.println(status);
    Serial.println(F("[HTTP][ETH][POST-PLAIN] Respuesta:"));
    Serial.println(response);
  }
  return (status >= 200 && status < 300);
}

// ============= GET crudo (headers + body) para sincronía de hora =============
bool httpGetRaw(const char* host, uint16_t port, const char* path,
                String &headersOut, String &bodyOut)
{
  headersOut = "";
  bodyOut    = "";

  if (Ethernet.localIP() == IPAddress(0,0,0,0)) {
    if (debugSerie) Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie) {
    Serial.print(F("[HTTP][ETH] Conectando "));
    Serial.print(host); Serial.print(':'); Serial.print(port); Serial.print(path);
    Serial.println(F(" ..."));
  }
  if (!client.connect(host, port)) {
    if (debugSerie) Serial.println(F("[HTTP] Fallo connect()"));
    return false;
  }

  // Petición mínima HTTP/1.1
  client.print(F("GET ")); client.print(path); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.print(host);
  if (port != 80) { client.print(':'); client.print(port); }
  client.println();
  client.println(F("Connection: close"));
  client.println();

  // Leer todo con timeout acumulado
  String raw; raw.reserve(1024);
  uint32_t t0 = millis();
  while (client.connected() || client.available()) {
    while (client.available()) {
      raw += (char)client.read();
      t0 = millis();
      if ((int)raw.length() > HTTP_MAX_BYTES) break;
    }
    if ((int)raw.length() > HTTP_MAX_BYTES) break;
    if (millis() - t0 > HTTP_READ_TIMEOUT_MS) break;
    delay(1);
  }
  client.stop();

  // Separar cabeceras y cuerpo
  int pos = raw.indexOf("\r\n\r\n");
  if (pos < 0) pos = raw.indexOf("\n\n");
  if (pos >= 0) {
    headersOut = raw.substring(0, pos);
    bodyOut    = raw.substring(pos + ((raw[pos] == '\r') ? 4 : 2));
  } else {
    headersOut = raw;
    bodyOut    = "";
  }

  // Comprobar 200 OK
  if (headersOut.startsWith("HTTP/1.1 200") || headersOut.startsWith("HTTP/1.0 200")) {
    return true;
  } else {
    if (debugSerie) Serial.println(F("[HTTP] Respuesta no 200"));
    return false;
  }
}

// ============= POST crudo (headers + body) si alguna vez lo necesitas =========
bool httpPostRaw(const char* host, uint16_t port, const char* path,
                 const String& payloadJSON,
                 String& outHeaders, String& outBody)
{
  outHeaders.remove(0); outBody.remove(0);

  if (Ethernet.localIP() == IPAddress(0,0,0,0)) {
    if (debugSerie) Serial.println(F("[HTTP] Ethernet sin IP."));
    return false;
  }

  EthernetClient client;
  client.setTimeout(HTTP_TIMEOUT_MS);

  if (debugSerie) {
    Serial.print(F("[HTTP][ETH] Conectando ")); Serial.print(host);
    Serial.print(':'); Serial.print(port); Serial.print(F(" ... "));
  }
  if (!client.connect(host, port)) {
    if (debugSerie) Serial.println(F("FALLO"));
    return false;
  }
  if (debugSerie) Serial.println(F("OK"));

  // ---- Envío petición ----
  client.print(F("POST ")); client.print(path); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.print(host); if (port != 80) { client.print(':'); client.print(port); } client.println();
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print  (F("Content-Length: ")); client.println(payloadJSON.length());
  client.println();
  client.print(payloadJSON);

  // ---- Status line ----
  String line;
  if (!readLine(client, line, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
  int sp1 = line.indexOf(' '), sp2 = (sp1>=0)? line.indexOf(' ', sp1+1) : -1;
  int status = (sp2>sp1)? line.substring(sp1+1, sp2).toInt() : 0;

  // ---- Headers completos ----
  bool chunked=false; size_t contentLen=0;
  for (;;) {
    if (!readLine(client, line, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
    if (line.length() == 0) break;
    outHeaders += line; outHeaders += "\r\n";
    String l = line; l.toLowerCase();
    if (l.startsWith(F("transfer-encoding:")) && l.indexOf(F("chunked"))>=0) chunked = true;
    else if (l.startsWith(F("content-length:"))) {
      int colon = line.indexOf(':');
      if (colon >= 0) contentLen = line.substring(colon+1).toInt();
    }
  }

  // ---- Body ----
  if (chunked) {
    for (;;) {
      String sz; if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; }
      sz.trim(); if (sz.length()==0) { if (!readLine(client, sz, HTTP_TIMEOUT_MS)) { client.stop(); return false; } sz.trim(); }
      unsigned long n = strtoul(sz.c_str(), nullptr, 16);
      if (n == 0) { readLine(client, line, HTTP_TIMEOUT_MS); break; }
      uint32_t t0 = millis();
      while (n > 0 && millis()-t0 < HTTP_TIMEOUT_MS) {
        while (client.available() && n>0) { outBody += (char)client.read(); n--; }
        if (n==0) break;
        delay(1);
      }
      readLine(client, line, HTTP_TIMEOUT_MS); // CRLF
    }
  } else if (contentLen > 0) {
    size_t rem = contentLen; uint32_t t0 = millis();
    while (rem > 0 && millis()-t0 < HTTP_TIMEOUT_MS) {
      while (client.available() && rem>0) { outBody += (char)client.read(); rem--; }
      if (rem==0) break;
      delay(1);
    }
  } else {
    while (client.connected() || client.available()) {
      while (client.available()) outBody += (char)client.read();
      delay(1);
    }
  }

  client.stop();
  if (debugSerie) {
    Serial.print(F("[HTTP][ETH] Código: ")); Serial.println(status);
    Serial.println(F("[HTTP][ETH] Headers ↓")); Serial.println(outHeaders);
    Serial.println(F("[HTTP][ETH] Body ↓"));    Serial.println(outBody);
  }
  return (status >= 200 && status < 300);
}

// ====================== API alto nivel (tu contrato) =========================

void getInicio() {
  serializaInicio();
  const String url = serverURL + "/status";
  String resp;
  bool ok = postJSON(url, outputInicio, resp);
  marcaHttpOk(ok);
  if (!ok && debugSerie) Serial.println(F("[API] ERROR: status inicio"));
  estadoRecibido = resp;
  descifraEstado();
}

void getEstado() {
  serializaEstado();
  const String url = serverURL + "/status";
  String resp;
  bool ok = postJSON(url, outputEstado, resp);
  marcaHttpOk(ok);
  if (!ok && debugSerie) Serial.println(F("[API] ERROR: status"));
  estadoRecibido = resp;
  descifraEstado();
}

void postTicket() {
  serializaQR();
  if (debugSerie) inicioServidor = millis();

  const String url  = serverURL + "/validateQR";
  String resp;
  bool okHttp2xx = postJSON(url, outputTicket, resp);
  marcaHttpOk(okHttp2xx);

  if (resp.length() > 0) {
    ticketRecibido = resp;
    descifraQR(); // fija g_validateOutcome (VAUTH_IN/OUT/VDENIED/VERROR) y estados
  } else {
    if (debugSerie) Serial.println(F("[API] validateQR sin cuerpo → VERROR y READY"));
    g_validateOutcome = VERROR;
    g_lastEd = "EMPTY_BODY";
    // fallback a READY
    activaConecta = 1; activaEntrada = 0; ultimoTicket = "";
    estadoMaquina = "CMD_READY"; estadoPuerta = "200";
  }

  if (debugSerie) {
    finServidor = millis();
    Serial.print(F("Tiempo en Servidor: "));
    Serial.print((finServidor - inicioServidor));
    Serial.println(F(" ms"));
  }
}

void postPaso() {
  serializaPaso();  // doc["barcode"] = ultimoPaso
  const String url = serverURL + "/validatePass";
  String resp;
  bool ok = postJSON(url, outputPaso, resp);
  marcaHttpOk(ok);

  if (!ok) {
    if (debugSerie) Serial.println(F("[API] ERROR: validatePass"));
    activaConecta = 1;
    activaEntrada = 0;
    return;
  }
  pasoRecibido = resp;
  descifraPaso();
}

void reportFailure() {
  serializaReportFailure();
  const String url = serverURL + "/reportFailure";
  String resp;
  bool ok = postJSON(url, outputInicio /* reutilizo buffer */, resp);
  marcaHttpOk(ok);
  if (!ok && debugSerie) Serial.println(F("[API] ERROR: reportFailure"));
  estadoRecibido = resp;
  descifraEstado();
}

// ================== ENTRADAS (texto plano) y PENDIENTES ======================

bool getEntradas(String& outTexto) {
  // Tu backend espera POST JSON a /entries y devuelve TEXTO PLANO
  serializaEstado();                        // Rellena 'outputEstado' con {id,status,ec}
  const String url = serverURL + "/entries";

  String resp;
  bool ok = postJSON(url, outputEstado, resp);
  marcaHttpOk(ok);
  if (!ok) return false;

  outTexto = resp;                          // "OK\n<code>:HH:MM;\n..."
  return true;
}

bool postPendientesBloque(const String& contenido) {
  const String url = serverURL + "/entries/pending"; // text/plain
  String resp;
  bool ok = postPlain(url, contenido, resp);
  marcaHttpOk(ok);
  return ok;
}

// ================== OTA por Ethernet (HTTP, sin TLS) =========================

void actualiza() {
  Serial.println(F("[OTA] Iniciando actualización por Ethernet..."));

  // 1) Solo HTTP con W5500
  if (String(URL_ACTUALIZA).startsWith("https://")) {
    Serial.println(F("[OTA] ABORTADO: URL https:// no soportada con W5500 (sin TLS). Usa http://"));
    return;
  }
  if (!String(URL_ACTUALIZA).startsWith("http://")) {
    Serial.println(F("[OTA] ABORTADO: URL inválida (debe empezar por http://)"));
    return;
  }

  // 2) Parsear URL -> host, port, path
  String host, path; uint16_t port;
  if (!parseHttpUrl(URL_ACTUALIZA, host, port, path)) {
    Serial.println(F("[OTA] ABORTADO: parseo de URL falló."));
    return;
  }
  Serial.print(F("[OTA] Descargando de ")); Serial.print(host);
  Serial.print(':'); Serial.print(port);
  Serial.print(' '); Serial.println(path);

  EthernetClient c;
  c.setTimeout(OTA_TIMEOUT_MS);

  // 3) Conexión TCP
  Serial.print(F("[OTA] Conectando... "));
  if (!c.connect(host.c_str(), port)) {
    Serial.println(F("FALLO"));
    return;
  }
  Serial.println(F("OK"));

  // 4) Petición GET
  c.print(F("GET ")); c.print(path); c.println(F(" HTTP/1.1"));
  c.print(F("Host: ")); c.println(host);
  c.println(F("Connection: close"));
  c.println();

  // 5) Status line
  String line;
  if (!readLine(c, line, OTA_TIMEOUT_MS)) { c.stop(); Serial.println(F("[OTA] Sin status line.")); return; }
  Serial.print(F("[OTA] Status: ")); Serial.println(line);
  if (!line.startsWith("HTTP/1.1 200") && !line.startsWith("HTTP/1.0 200")) {
    c.stop();
    Serial.println(F("[OTA] ABORTADO: servidor no devolvió 200."));
    return;
  }

  // 6) Headers -> Content-Length (obligatorio) y opcional X-MD5
  size_t contentLen = 0;
  String md5;
  while (readLine(c, line, OTA_TIMEOUT_MS)) {
    if (line.length() == 0) break; // fin de cabeceras
    if (line.startsWith(F("Content-Length:"))) {
      contentLen = (size_t) line.substring(15).toInt();
    } else if (line.startsWith(F("X-MD5:"))) {
      md5 = line.substring(6); md5.trim();
    }
  }

  if (contentLen == 0) {
    c.stop();
    Serial.println(F("[OTA] ABORTADO: Content-Length ausente o 0."));
    return;
  }

  // 7) Preparar Update
  if (md5.length() == 32) {
    if (!Update.setMD5(md5.c_str())) {
      Serial.println(F("[OTA] Aviso: setMD5() falló; se continúa sin verificación."));
    } else {
      Serial.print(F("[OTA] MD5 esperado: ")); Serial.println(md5);
    }
  }

  Serial.print(F("[OTA] Tamaño: ")); Serial.print(contentLen); Serial.println(F(" bytes"));
  if (!Update.begin(contentLen)) {
    c.stop();
    Serial.printf("[OTA] Update.begin() falló. Error=%d\n", Update.getError());
    return;
  }

  // 8) Stream a flash
  size_t written = 0;
  uint8_t buf[2048];
  while ((written < contentLen) && (c.connected() || c.available())) {
    int n = c.readBytes(buf, sizeof(buf));
    if (n > 0) {
      size_t w = Update.write(buf, (size_t)n);
      if (w != (size_t)n) {
        Serial.printf("[OTA] Error escribiendo en flash. w=%u n=%d Err=%d\n",
                      (unsigned)w, n, Update.getError());
        Update.end();
        c.stop();
        return;
      }
      written += w;
    } else {
      delay(1);
    }
  }
  c.stop();

  // 9) Finalizar
  if (!Update.end()) {
    Serial.printf("[OTA] Update.end() falló. Err=%d\n", Update.getError());
    return;
  }
  if (written != contentLen) {
    Serial.printf("[OTA] Incompleto: escritos=%u de=%u\n", (unsigned)written, (unsigned)contentLen);
    return;
  }
  if (!Update.isFinished()) {
    Serial.println(F("[OTA] No finalizó correctamente."));
    return;
  }

  Serial.println(F("[OTA] Actualización OK. Reiniciando..."));
  delay(250);
  ESP.restart();
}
