#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "http.hpp"
#include "json.hpp"
#include "config.hpp"

// --- servidor y cola hacia IO ---
static WebServer      server(8080);
static QueueHandle_t  g_qToIO = nullptr;

// ====== Helper interno: POST JSON (HTTPS “inseguro” en desarrollo) ======


// ================== WiFi: IP estática y conexión ==================
void begin_wifi() {
  WiFi.mode(WIFI_STA);

  // IP estática (debe ir antes de begin())
  bool ok = WiFi.config(LOCAL_IP, GATEWAY, SUBNET, DNS1, DNS2);
  if (!ok) Serial.println("[WIFI] WiFi.config() FALLÓ");

  // (Opcional) Hostname útil en router
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

// ================== POST: implementación base (4 args) ==================
static bool post(const String& url,
                 const String& payload,
                 String& response,
                 uint16_t timeout_ms)
{
  response.clear();

  if (WiFi.status() != WL_CONNECTED) {
    if (debugSerie) Serial.println("[HTTP] WiFi no conectado");
    return false;
  }

  // Limpia posibles espacios
  String cleanUrl = url;
  cleanUrl.trim();

  const bool isHttps = cleanUrl.startsWith("https://");
  HTTPClient http;
  bool began = false;

  if (isHttps) {
    WiFiClientSecure client;
    client.setInsecure(); // SOLO DESARROLLO. En producción: client.setCACert(...)
    began = http.begin(client, cleanUrl);
  } else {
    WiFiClient client;
    began = http.begin(client, cleanUrl);
    // Alternativa: began = http.begin(cleanUrl);
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
  http.addHeader("Connection", "close"); // evita sockets colgados en ciertos routers

  if (debugSerie) {
    Serial.println("[HTTP] POST -> " + cleanUrl);
    Serial.println("[HTTP] Payload:");
    Serial.println(payload);
  }

  // Usa el overload de String (evita const-cast problem)
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

// ================== POST: overload conveniente (3 args) ==================
static bool post(const String& url,
                 const String& payload,
                 String& response)
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
  // 'recibidoUDP' -> usado por serializaPaso()
  serializaPaso();

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
  if (!post(url, outputInicio /* reutilizas buffer */, resp)) {
    if (debugSerie) Serial.println("[API] ERROR: reportFailure");
  }
  estadoRecibido = resp;
  descifraEstado();
}

// ================== Servidor HTTP embebido (endpoints locales) ==================
static void handle_root() {
  server.send(200, "text/plain", "OK");
}

static void handle_slave_exit() {
  String body = server.arg("plain");
  if (debugSerie) {
    Serial.println("[HTTP] /slave/exit body:");
    Serial.println(body);
  }

  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, body);
  if (e) {
    server.send(400, "application/json", "{\"error\":\"json invalid\"}");
    return;
  }

  const String comercio = doc["comercio"] | "";
  const String id       = doc["ID"]       | "";
  const String status   = doc["status"]   | "";
  const String ec       = doc["ec"]       | "";
  const String barcode  = doc["barcode"]  | "";

  // Validaciones mínimas
  if (comercio != COMERCIO || id != DEVICE_ID || status != "202" || barcode.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"fields invalid\"}");
    return;
  }

  // Encolar orden hacia IO
  CmdMsg m;
  m.type = CMD_SLAVE_EXIT;
  strlcpy(m.payload, barcode.c_str(), sizeof(m.payload));

  if (xQueueSend(g_qToIO, &m, 0) != pdTRUE) {
    server.send(503, "application/json", "{\"error\":\"queue full\"}");
    return;
  }

  // Marca salida activa para bloquear GM65 inmediatamente
  activaSalida = 1;

  // Respuesta inmediata
  server.send(200, "application/json", "{\"R\":\"OK\"}");
}

void http_begin(QueueHandle_t qToIO) {
  g_qToIO = qToIO;
  server.on("/", HTTP_GET, handle_root);
  server.on("/slave/exit", HTTP_POST, handle_slave_exit);
  server.begin();
  if (debugSerie) Serial.println("[HTTP] Servidor iniciado en :8080");
}

void http_poll() {
  server.handleClient();
}

// ================== OTA ==================
void actualiza() {
  Serial.println("Actualizando");
  delay(1000);

  WiFiClientSecure cliente;
  cliente.setInsecure(); // SOLO desarrollo. En producción: setCACert(...)

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