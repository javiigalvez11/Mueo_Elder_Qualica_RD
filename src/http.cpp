// ========================== http.cpp ==========================
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#include "http.hpp"
#include "json.hpp"     // declara buffers/strings/flags/externs
#include "config.hpp"   // DEVICE_ID, COMERCIO, serverURL, etc.

// --- servidor y cola hacia IO ---
static WebServer      server(8080);
static QueueHandle_t  g_qToIO = nullptr;

// ================== WiFi: IP estática y conexión ==================
void begin_wifi() {
  WiFi.mode(WIFI_STA);

  bool ok = WiFi.config(LOCAL_IP, GATEWAY, SUBNET, DNS1, DNS2);
  if (!ok) Serial.println("[WIFI] WiFi.config() FALLÓ");

  WiFi.setHostname(DEVICE_ID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Conectando a %s …\n", WIFI_SSID);

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries++ < 60) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Conectado.");
    Serial.print("[WIFI] IP:    "); Serial.println(WiFi.localIP());
    Serial.print("[WIFI] GW:    "); Serial.println(WiFi.gatewayIP());
    Serial.print("[WIFI] MASK:  "); Serial.println(WiFi.subnetMask());
    Serial.print("[WIFI] DNS1:  "); Serial.println(WiFi.dnsIP(0));
    Serial.print("[WIFI] DNS2:  "); Serial.println(WiFi.dnsIP(1));
  } else {
    Serial.println("[WIFI] No se pudo conectar");
  }
}

// ================== POST: implementación base ==================
static bool post(const String& url, const String& payload, String& response, uint16_t timeout_ms)
{
  response.clear();

  if (WiFi.status() != WL_CONNECTED) {
    if (debugSerie) Serial.println("[HTTP] WiFi no conectado");
    return false;
  }

  String cleanUrl = url; cleanUrl.trim();

  const bool isHttps = cleanUrl.startsWith("https://");
  HTTPClient http;
  bool began = false;

  if (isHttps) {
    WiFiClientSecure client;
    client.setInsecure(); // SOLO DESARROLLO
    began = http.begin(client, cleanUrl);
  } else {
    WiFiClient client;
    began = http.begin(client, cleanUrl);
  }

  if (!began) {
    if (debugSerie) {
      Serial.println("[HTTP] begin() falló");
      Serial.println(String("[HTTP] URL -> '") + cleanUrl + "'");
    }
    return false;
  }

  http.setTimeout(timeout_ms);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  if (debugSerie) {
    Serial.println("[HTTP] POST -> " + cleanUrl);
    Serial.println("[HTTP] Payload:");
    Serial.println(payload);
  }

  const int code = http.POST(payload);

  if (code > 0) {
    response = http.getString();
    if (debugSerie) {
      Serial.print("[HTTP] Código: "); Serial.println(code);
      Serial.println("[HTTP] Respuesta:");
      Serial.println(response);
    }
  } else {
    if (debugSerie) {
      Serial.print("[HTTP] Error POST: ");
      Serial.println(http.errorToString(code));
    }
  }

  http.end();
  return (code >= 200 && code < 300);
}

static bool post(const String& url, const String& payload, String& response)
{
  return post(url, payload, response, HTTP_TIMEOUT_MS);
}

// ================== API CLIENT ==================
void getInicio() {
  serializaInicio();
  const String url = serverURL + "/status";
  String resp;
  if (!post(url, outputInicio, resp)) {
    if (debugSerie) Serial.println("[API] ERROR: status inicio");
  }
  estadoRecibido = resp;
  descifraEstado();
}

void getEstado() {
  serializaEstado();
  const String url = serverURL + "/status";
  String resp;
  if (!post(url, outputEstado, resp)) {
    if (debugSerie) Serial.println("[API] ERROR: status");
  }
  estadoRecibido = resp;
  descifraEstado();
}

void postTicket() {
  serializaQR();

  if (debugSerie) inicioServidor = millis();

  const String url  = serverURL + "/validateQR";
  String resp;
  if (!post(url, outputTicket, resp)) {
    if (debugSerie) Serial.println("[API] ERROR: validateQR");
    activaConecta = 1;  // reactiva /status
    activaEntrada = 0;  // reactiva entrada
    return;
  }

  ticketRecibido = resp;
  descifraQR();

  if (debugSerie) {
    finServidor = millis();
    Serial.print("Tiempo en Servidor: ");
    Serial.print((finServidor - inicioServidor));
    Serial.println(" ms");
  }
}

void postPaso() {
  serializaPaso();  // Asegúrate: doc["barcode"] = ultimoPaso (SOLO el código)
  const String url = serverURL + "/validatePass";
  String resp;
  if (!post(url, outputPaso, resp)) {
    if (debugSerie) Serial.println("[API] ERROR: validatePass");
    return;
  }
  pasoRecibido = resp;
  descifraPaso();
}

void reportFailure() {
  serializaReportFailure();
  const String url = serverURL + "/reportFailure";
  String resp;
  if (!post(url, outputInicio /* reutilizo buffer */, resp)) {
    if (debugSerie) Serial.println("[API] ERROR: reportFailure");
  }
  estadoRecibido = resp;
  descifraEstado();
}

// ================== Servidor HTTP embebido ==================
static void handle_root() {
  server.sendHeader("Connection","close");
  server.send(200, "text/plain", "OK");
}

// Debounce por ruta
static unsigned long s_lastEntryMs = 0;
static String        s_lastEntryCode;
static unsigned long s_lastExitMs  = 0;
static String        s_lastExitCode;

// Extrae SOLO el código (soporta JSON o text/plain)
static bool extract_code(const String& body, String& outCode) {
  outCode = "";
  if (body.isEmpty()) return false;

  if (body[0] == '{') {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) return false;
    outCode = (const char*)(doc["barcode"] | "");
    outCode.trim();
    return outCode.length() > 0;
  } else {
    outCode = body; outCode.trim();
    return outCode.length() > 0;
  }
}

// /reader/scan  -> recibe el código en body (texto plano) y lo trata como ENTRADA
static void handle_reader_scan() {
  String body = server.arg("plain");   // p.ej. "ME00151020..."
  if (debugSerie) {
    Serial.println("[HTTP] /reader/scan body:");
    Serial.println(body);
  }
  String code;
  if (!extract_code(body, code)) {
    server.sendHeader("Connection","close");
    server.send(400, "application/json", "{\"error\":\"invalid body\"}");
    return;
  }

  CmdMsg m; m.type = CMD_SLAVE_ENTRY; strlcpy(m.payload, code.c_str(), sizeof(m.payload));
  xQueueSend(g_qToIO, &m, 0);

  server.sendHeader("Connection","close");
  server.send(200, "application/json", "{\"R\":\"OK\"}");
}

// /slave/entry -> activa ENTRADA (acepta JSON o text/plain) con dedupe
static void handle_slave_entry() {
  const String body = server.arg("plain");
  if (debugSerie) { Serial.println("[HTTP] /slave/entry body:"); Serial.println(body); }

  String code;
  if (!extract_code(body, code)) {
    server.sendHeader("Connection","close");
    server.send(400, "application/json", "{\"error\":\"invalid body\"}");
    return;
  }

  const unsigned long now = millis();
  if (code == s_lastEntryCode && (now - s_lastEntryMs) < 700) {
    if (debugSerie) Serial.println("[HTTP] /slave/entry DEDUP ignorado");
    server.sendHeader("Connection","close");
    server.send(200, "application/json", "{\"R\":\"OK\",\"dedup\":true}");
    return;
  }
  s_lastEntryCode = code; s_lastEntryMs = now;

  CmdMsg m; m.type = CMD_SLAVE_ENTRY; strlcpy(m.payload, code.c_str(), sizeof(m.payload));
  if (xQueueSend(g_qToIO, &m, 0) != pdTRUE) {
    server.sendHeader("Connection","close");
    server.send(503, "application/json", "{\"error\":\"queue full\"}");
    return;
  }

  activaEntrada = 1;
  server.sendHeader("Connection","close");
  server.send(200, "application/json", "{\"R\":\"OK\"}");
}

// /slave/exit  -> activa SALIDA (acepta JSON o text/plain) con dedupe
static void handle_slave_exit() {
  const String body = server.arg("plain");
  if (debugSerie) { Serial.println("[HTTP] /slave/exit body:"); Serial.println(body); }

  String code;
  if (!extract_code(body, code)) {
    server.sendHeader("Connection","close");
    server.send(400, "application/json", "{\"error\":\"invalid body\"}");
    return;
  }

  const unsigned long now = millis();
  if (code == s_lastExitCode && (now - s_lastExitMs) < 700) {
    if (debugSerie) Serial.println("[HTTP] /slave/exit DEDUP ignorado");
    server.sendHeader("Connection","close");
    server.send(200, "application/json", "{\"R\":\"OK\",\"dedup\":true}");
    return;
  }
  s_lastExitCode = code; s_lastExitMs = now;

  CmdMsg m; m.type = CMD_SLAVE_EXIT; strlcpy(m.payload, code.c_str(), sizeof(m.payload));
  if (xQueueSend(g_qToIO, &m, 0) != pdTRUE) {
    server.sendHeader("Connection","close");
    server.send(503, "application/json", "{\"error\":\"queue full\"}");
    return;
  }

  activaSalida = 1;
  server.sendHeader("Connection","close");
  server.send(200, "application/json", "{\"R\":\"OK\"}");
}

// 404 simple (útil para depurar)
static void handle_not_found() {
  if (debugSerie) {
    Serial.printf("[HTTP] 404 %s %s  body:'%s'\n",
                  (server.method()==HTTP_GET)?"GET":(server.method()==HTTP_POST)?"POST":"OTHER",
                  server.uri().c_str(),
                  server.arg("plain").c_str());
  }
  server.sendHeader("Connection","close");
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

// --- Registro de rutas ---
void http_begin(QueueHandle_t qToIO) {
  g_qToIO = qToIO;

  const char* H[] = {"Content-Type"};
  server.collectHeaders(H, 1);

  server.on("/",            HTTP_GET,  handle_root);
  server.on("/reader/scan", HTTP_POST, handle_reader_scan);
  server.on("/slave/exit",  HTTP_POST, handle_slave_exit);
  server.on("/slave/entry", HTTP_POST, handle_slave_entry);

  // Endpoints de prueba rápidos (curl GET)
  server.on("/test/entry", HTTP_GET, [](){
    CmdMsg m; m.type = CMD_SLAVE_ENTRY; m.payload[0] = '\0';
    xQueueSend(g_qToIO, &m, 0);
    server.sendHeader("Connection","close");
    server.send(200, "text/plain", "entry ok");
  });
  server.on("/test/exit", HTTP_GET, [](){
    CmdMsg m; m.type = CMD_SLAVE_EXIT; m.payload[0] = '\0';
    xQueueSend(g_qToIO, &m, 0);
    server.sendHeader("Connection","close");
    server.send(200, "text/plain", "exit ok");
  });

  server.onNotFound(handle_not_found);

  server.begin();
  if (debugSerie) Serial.println("[HTTP] Servidor iniciado");
}

void http_poll() { server.handleClient(); }

// ================== OTA ==================
void actualiza() {
  Serial.println("Actualizando");
  delay(1000);

  WiFiClientSecure cliente;
  cliente.setInsecure(); // SOLO desarrollo

  t_httpUpdate_return ret = httpUpdate.update(cliente, urlActualiza);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED (%d): %s\n",
        httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

// ======================== fin http.cpp ========================
