// web_eth.cpp
#include "web_eth.hpp"


// ========================= Helpers HTTP =========================

static void sendResponse(EthernetClient &client, int code,
                         const String &contentType,
                         const String &body,
                         const String &extraHeaders = "")
{
  // Línea de estado
  client.print("HTTP/1.1 ");
  client.print(code);
  switch (code)
  {
  case 200:
    client.print(" OK");
    break;
  case 302:
    client.print(" Found");
    break;
  case 400:
    client.print(" Bad Request");
    break;
  case 401:
    client.print(" Unauthorized");
    break;
  case 404:
    client.print(" Not Found");
    break;
  case 500:
    client.print(" Internal Server Error");
    break;
  default:
    client.print(" OK");
    break;
  }
  client.print("\r\n");

  // Cabeceras extra
  if (extraHeaders.length() > 0)
  {
    client.print(extraHeaders);
    if (!extraHeaders.endsWith("\r\n"))
      client.print("\r\n");
  }

  // Content-Type (si hay body)
  if (body.length() > 0)
  {
    client.print("Content-Type: ");
    client.print(contentType);
    client.print("\r\n");
  }

  // Content-Length siempre
  client.print("Content-Length: ");
  client.print((unsigned long)body.length());
  client.print("\r\n");

  client.print("Connection: close\r\n");
  client.print("\r\n");

  if (body.length() > 0)
  {
    const char *buf = body.c_str();
    size_t len = (size_t)body.length();
    size_t written = 0;

    while (written < len && client.connected())
    {
      size_t chunk = len - written;
      if (chunk > 1024)
        chunk = 1024;

      int w = client.write((const uint8_t *)(buf + written), chunk);
      if (w <= 0)
        break;

      written += (size_t)w;

      vTaskDelay(pdMS_TO_TICKS(1)); // O delay(1) evita bloquear mucho tiempo el loop de Ethernet
    }
    client.flush();

    if (debugSerie)
    {
      Serial.print("sendResponse: body bytes written=");
      Serial.println((unsigned long)written);
    }
  }
}

// Lee la primera línea del request (método + ruta + versión)
static bool readRequestLine(EthernetClient &client, String &method, String &path, String &version)
{
  String line = client.readStringUntil('\n');
  line.trim();
  if (line.length() == 0)
    return false;

  int firstSpace = line.indexOf(' ');
  int secondSpace = line.indexOf(' ', firstSpace + 1);
  if (firstSpace < 0 || secondSpace < 0)
    return false;

  method = line.substring(0, firstSpace);
  path = line.substring(firstSpace + 1, secondSpace);
  version = line.substring(secondSpace + 1);
  return true;
}

// Lee cabeceras; devuelve Content-Length y Content-Type si existe
static int readHeaders(EthernetClient &client, String &outContentType)
{
  int contentLength = 0;
  outContentType = "";

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      break; // fin headers

    int colon = line.indexOf(':');
    if (colon < 0)
      continue;

    String headerName = line.substring(0, colon);
    headerName.trim();
    headerName.toLowerCase();

    String value = line.substring(colon + 1);
    value.trim();

    if (headerName == "content-length")
      contentLength = value.toInt();
    else if (headerName == "content-type")
      outContentType = value;
  }
  return contentLength;
}

// Lee body según Content-Length
static String readBody(EthernetClient &client, int contentLength)
{
  String body;
  body.reserve(contentLength);

  while (contentLength > 0 && client.connected())
  {
    while (client.available() && contentLength > 0)
    {
      char c = client.read();
      body += c;
      contentLength--;
    }
  }
  return body;
}

static String getParam(const String &body, const String &key)
{
  String needle = key + "=";
  int p = body.indexOf(needle);
  if (p < 0)
    return "";

  int start = p + needle.length();
  int end = body.indexOf('&', start);
  if (end < 0)
    end = body.length();

  return urlDecode(body.substring(start, end));
}

static inline bool requireAuth(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return false;
  }
  lastActivityTime_eth = millis();
  return true;
}

// ========================= Status UI (profesional) =========================

static void redirectStatus(EthernetClient &client,
                           const String &kind,
                           const String &title,
                           const String &msg,
                           const String &backUrl,
                           const String &backText,
                           const String &redirUrl = "",
                           int redirMs = 0)
{
  String loc = "/status?kind=" + urlEncode(kind) +
               "&title=" + urlEncode(title) +
               "&msg=" + urlEncode(msg) +
               "&back=" + urlEncode(backUrl) +
               "&backText=" + urlEncode(backText);

  if (redirUrl.length() > 0 && redirMs > 0)
  {
    loc += "&redir=" + urlEncode(redirUrl);
    loc += "&ms=" + urlEncode(String(redirMs));
  }

  sendResponse(client, 302, "text/plain", "", "Location: " + loc + "\r\n");
}

static void handleStatus(EthernetClient &client, const String &pathWithQuery)
{
  File file = LittleFS.open("/status.html", "r");
  if (!file)
  {
    // fallback
    sendResponse(client, 500, "text/plain", "status.html no encontrado en LittleFS");
    return;
  }

  String html = file.readString();
  file.close();

  String kind = getQueryParam(pathWithQuery, "kind");
  if (kind != "success" && kind != "error" && kind != "info")
    kind = "info";

  String title = getQueryParam(pathWithQuery, "title");
  if (title.length() == 0)
    title = (kind == "success") ? "Éxito" : (kind == "error") ? "Error"
                                                              : "Información";

  String msg = getQueryParam(pathWithQuery, "msg");
  if (msg.length() == 0)
    msg = "Operación completada.";

  String back = getQueryParam(pathWithQuery, "back");
  if (back.length() == 0)
    back = "/menu";

  String backText = getQueryParam(pathWithQuery, "backText");
  if (backText.length() == 0)
    backText = "Volver";

  String redir = getQueryParam(pathWithQuery, "redir");
  String ms = getQueryParam(pathWithQuery, "ms");

  html.replace("{{KIND}}", htmlEscape(kind));
  html.replace("{{TITLE}}", htmlEscape(title));
  html.replace("{{MESSAGE}}", htmlEscape(msg));
  html.replace("{{BACK_URL}}", htmlEscape(back));
  html.replace("{{BACK_TEXT}}", htmlEscape(backText));
  html.replace("{{AUTO_REDIRECT_URL}}", htmlEscape(redir));
  html.replace("{{AUTO_REDIRECT_MS}}", htmlEscape(ms));

  // Texto auxiliar
  if (redir.length() == 0 || ms.length() == 0)
    html.replace("{{REDIRECT_HINT}}", "");
  else
    html.replace("{{REDIRECT_HINT}}", "Redirigiendo automáticamente...");

  sendResponse(client, 200, "text/html; charset=utf-8", html);
}

// ========================= Static files =========================

static String contentTypeFromPath(const String &path)
{
  if (path.endsWith(".html"))
    return "text/html; charset=utf-8";
  if (path.endsWith(".css"))
    return "text/css";
  if (path.endsWith(".js"))
    return "application/javascript";
  if (path.endsWith(".png"))
    return "image/png";
  if (path.endsWith(".jpg"))
    return "image/jpeg";
  if (path.endsWith(".jpeg"))
    return "image/jpeg";
  if (path.endsWith(".svg"))
    return "image/svg+xml";
  if (path.endsWith(".ico"))
    return "image/x-icon";
  if (path.endsWith(".json"))
    return "application/json";
  return "application/octet-stream";
}

static void sendFile(EthernetClient &client, const String &path)
{
  File file = LittleFS.open(path, "r");
  if (!file)
  {
    redirectStatus(client, "error", "Recurso no disponible",
                   "El archivo solicitado no existe en el dispositivo.",
                   "/menu", "Volver al menú");
    return;
  }

  String contentType = contentTypeFromPath(path);

  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: ");
  client.print(contentType);
  client.print("\r\n");
  client.print("Content-Length: ");
  client.print((unsigned long)file.size());
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");

  uint8_t buf[512];
  while (file.available())
  {
    size_t r = file.read(buf, sizeof(buf));
    if (r)
      client.write(buf, r);
  }
  file.close();
}

// ========================= Rutas =========================

static void mensajeError(EthernetClient &client)
{
  redirectStatus(client, "error", "404 - Página no encontrada",
                 "La ruta solicitada no existe. Revisa la URL o vuelve al menú.",
                 "/menu", "Volver al menú");
}

void handleRoot(EthernetClient &client)
{
  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    if (debugSerie)
      Serial.println("handleRoot: index.html no encontrado en LittleFS");
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /index.html en LittleFS. Sube el sistema de ficheros.",
                   "/upload_fs", "Ir a actualizar FS");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  htmlContent.replace("{{ERROR_MSG}}", errorMessage_eth);
  errorMessage_eth = "";

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleSubmit(EthernetClient &client, const String &body)
{
  String pwd;
  int idx = body.indexOf("password=");
  if (idx >= 0)
  {
    int start = idx + 9;
    int end = body.indexOf('&', start);
    if (end < 0)
      end = body.length();
    pwd = urlDecode(body.substring(start, end));
  }

  if (pwd.length() == 0)
  {
    errorMessage_eth = "<span style=\"color:red;\">Por favor, introduzca una contraseña.</span>";
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }

  if (pwd == credencialMaestra)
  {
    registrado_eth = true;
    lastActivityTime_eth = millis();
    errorMessage_eth = "";

    logbuf_clear();      // Limpiar buffer de logs
    logbuf_enable(true); // CLAVE: desde aquí se loguea

    sendResponse(client, 302, "text/plain", "", "Location: /menu\r\n");
  }
  else
  {
    errorMessage_eth = "<span style=\"color:red;\">Contraseña incorrecta, reintente.</span>";
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
  }
}

void handleMenu(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }

  lastActivityTime_eth = millis();
  File file = LittleFS.open("/menu.html", "r");
  if (!file)
  {
    if (debugSerie)
      Serial.println("handleMenu: menu.html no encontrado en LittleFS");
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /menu.html en LittleFS. Sube el sistema de ficheros.",
                   "/upload_fs", "Ir a actualizar FS");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleReiniciarPage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }

  File file = LittleFS.open("/reiniciar.html", "r");
  if (!file)
  {
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /reiniciar.html en LittleFS.",
                   "/menu", "Volver al menú");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleReiniciarDispositivo(EthernetClient &client)
{
  if (!registrado_eth)
  {
    redirectStatus(client, "error", "Acceso denegado",
                   "No tienes sesión iniciada para reiniciar el dispositivo.",
                   "/", "Volver al login");
    return;
  }

  lastActivityTime_eth = millis();

  // Página profesional + reinicio
  redirectStatus(client, "info", "Reinicio",
                 "Se ha solicitado el reinicio. El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  delay(150);
  ESP.restart();
}

void handleFirmwarePage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }

  lastActivityTime_eth = millis();

  File file = LittleFS.open("/upload.html", "r");
  if (!file)
  {
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /upload.html en LittleFS.",
                   "/menu", "Volver al menú");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  htmlContent.replace("VERSION_FIRMWARE", enVersion);

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleLogout(EthernetClient &client)
{
  registrado_eth = false;
  errorMessage_eth = "";

  logbuf_enable(false); // CLAVE: deja de loguear
  logbuf_clear();       // opcional

  sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
}

// ========================= Upload firmware (multipart/form-data) =========================

void handleFirmwareUpload(EthernetClient &client, int contentLength, const String &contentType)
{
  if (debugSerie)
    Serial.println("handleFirmwareUpload: inicio");
  if (!client)
    return;

  if (!registrado_eth)
  {
    redirectStatus(client, "error", "Acceso denegado",
                   "Inicia sesión para poder actualizar el firmware.",
                   "/", "Volver al login");
    return;
  }

  // boundary
  String boundary = "";
  int bpos = contentType.indexOf("boundary=");
  if (bpos >= 0)
  {
    boundary = contentType.substring(bpos + 9);
    boundary.trim();
  }
  if (boundary.length() == 0)
  {
    redirectStatus(client, "error", "Solicitud inválida",
                   "No se detectó 'boundary' en Content-Type. Reintenta la subida.",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  String boundaryMarker = "--" + boundary;

  int remaining = contentLength;

  // leer cabeceras de la parte
  String partHeaders;
  while (remaining > 0 && client.connected())
  {
    char c = client.read();
    remaining--;
    partHeaders += c;
    if (partHeaders.endsWith("\r\n\r\n"))
      break;
  }

  // filename
  String filename;
  int fnPos = partHeaders.indexOf("filename=");
  if (fnPos >= 0)
  {
    int start = partHeaders.indexOf('"', fnPos);
    if (start >= 0)
    {
      int end = partHeaders.indexOf('"', start + 1);
      if (end > start)
        filename = partHeaders.substring(start + 1, end);
    }
  }

  // normalizar rutas
  int slash = filename.lastIndexOf('/');
  int bslash = filename.lastIndexOf('\\');
  int cut = max(slash, bslash);
  if (cut >= 0)
    filename = filename.substring(cut + 1);
  filename.trim();

  // validar nombre

  if (filename != DEVICE_ID + ".bin")
  {
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    redirectStatus(client, "error", "Archivo no válido",
                   "El fichero debe llamarse exactamente: " + DEVICE_ID + ".bin",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  // temporal en FS
  const char *tmpPath = "/upload.bin";
  if (LittleFS.exists(tmpPath))
    LittleFS.remove(tmpPath);

  File f = LittleFS.open(tmpPath, "w");
  if (!f)
  {
    redirectStatus(client, "error", "Error de almacenamiento",
                   "No se pudo crear el archivo temporal en LittleFS.",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  // recibir cuerpo detectando boundary
  String tail;
  const size_t BUF_SZ = 512;
  uint8_t buf[BUF_SZ];

  while (remaining > 0 && client.connected())
  {
    size_t toRead = remaining;
    if (toRead > BUF_SZ)
      toRead = BUF_SZ;
    int r = client.readBytes(buf, toRead);
    if (r <= 0)
      break;
    remaining -= r;

    String chunk;
    chunk.reserve(r);
    for (int i = 0; i < r; ++i)
      chunk += (char)buf[i];

    String combined = tail + chunk;
    int idxB = combined.indexOf(boundaryMarker);

    if (idxB >= 0)
    {
      int writeUpTo = idxB;
      if (writeUpTo >= 2 &&
          combined.charAt(writeUpTo - 2) == '\r' &&
          combined.charAt(writeUpTo - 1) == '\n')
      {
        writeUpTo -= 2;
      }
      for (int i = 0; i < writeUpTo; ++i)
        f.write((uint8_t)combined.charAt(i));
      break;
    }
    else
    {
      int keep = boundaryMarker.length();
      if ((int)combined.length() <= keep)
      {
        tail = combined;
        continue;
      }
      int writeLen = combined.length() - keep;
      for (int i = 0; i < writeLen; ++i)
        f.write((uint8_t)combined.charAt(i));
      tail = combined.substring(writeLen);
    }
  }

  f.close();

  File f2 = LittleFS.open(tmpPath, "r");
  if (!f2)
  {
    redirectStatus(client, "error", "Error de lectura",
                   "No se pudo abrir el firmware temporal para validar.",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  size_t fwSize = f2.size();
  if (fwSize == 0)
  {
    f2.close();
    LittleFS.remove(tmpPath);
    redirectStatus(client, "error", "Fichero vacío",
                   "No se recibieron datos válidos. Reintenta la subida.",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (fwSize > maxSketchSpace)
  {
    f2.close();
    LittleFS.remove(tmpPath);
    redirectStatus(client, "error", "Firmware demasiado grande",
                   "El firmware excede el espacio disponible para OTA.",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  if (!Update.begin(fwSize))
  {
    if (debugSerie)
      Serial.printf("Update.begin failed: %s\n", Update.errorString());
    f2.close();
    LittleFS.remove(tmpPath);
    redirectStatus(client, "error", "Error de actualización",
                   "No se pudo iniciar la actualización (Update.begin falló).",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  size_t written = 0;
  uint8_t rb[512];

  while (f2.available())
  {
    int r = f2.read(rb, sizeof(rb));
    if (r > 0)
    {
      size_t w = Update.write(rb, r);
      if (w != (size_t)r)
      {
        Update.end();
        f2.close();
        LittleFS.remove(tmpPath);
        redirectStatus(client, "error", "Error de escritura",
                       "Fallo escribiendo el firmware en flash.",
                       "/upload_firmware", "Volver a firmware");
        return;
      }
      written += w;
    }
  }
  f2.close();

  bool ok = Update.end(true);
  if (!ok)
  {
    if (debugSerie)
      Serial.printf("Update end error: %s\n", Update.errorString());
    LittleFS.remove(tmpPath);
    redirectStatus(client, "error", "Actualización fallida",
                   "El firmware no se pudo finalizar correctamente.",
                   "/upload_firmware", "Volver a firmware");
    return;
  }

  LittleFS.remove(tmpPath);

  // respuesta profesional + reinicio
  redirectStatus(client, "success", "Firmware actualizado",
                 "Firmware cargado correctamente. El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  delay(150);
  ESP.restart();
}

// ========================= Config (Preferences) =========================
void handleConfigPage(EthernetClient &client)
{
  if (!requireAuth(client))
    return;

  File file = LittleFS.open("/config.html", "r");
  if (!file)
  {
    redirectStatus(client, "error", "Error", "Falta config.html", "/menu", "Volver");
    return;
  }
  String html = file.readString();
  file.close();

  TornoConfig c = cfgLoad();

  // --- LÓGICA DE RED DINÁMICA ---
  String currentIP, currentGW, currentMask, currentDNS1, currentDNS2;

  if (c.modoRed == 0) // DHCP Activo: leemos la realidad del hardware
  {
    currentIP   = Ethernet.localIP().toString();
    currentGW   = Ethernet.gatewayIP().toString();
    currentMask = Ethernet.subnetMask().toString();
    currentDNS1 = Ethernet.dnsServerIP().toString();
    currentDNS2 = "0.0.0.0"; // La librería Ethernet estándar suele reportar un solo DNS
  }
  else // IP Estática: mostramos lo que hay guardado en la configuración
  {
    currentIP   = c.ip;
    currentGW   = c.gw;
    currentMask = c.mask;
    currentDNS1 = c.dns1;
    currentDNS2 = c.dns2;
  }

  // Identificación
  html.replace("{{DEVICE_ID}}", htmlEscape(c.deviceId));
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", c.mac[0], c.mac[1], c.mac[2], c.mac[3], c.mac[4], c.mac[5]);
  html.replace("{{MAC_ADDRESS}}", String(macStr));

  // Hardware, Sentido y Apertura (Asegúrate de que estas etiquetas coinciden con tu config.html)
  html.replace("{{MOD_VEGA_SEL}}", (c.modoPasillo == 0) ? "selected" : "");
  html.replace("{{MOD_CANOPU_SEL}}", (c.modoPasillo == 1) ? "selected" : "");
  html.replace("{{MOD_ARTURUS_SEL}}", (c.modoPasillo == 2) ? "selected" : "");
  
  html.replace("{{MODO_RS485_SEL}}", (c.modoApertura == 0) ? "selected" : "");
  html.replace("{{MODO_RELES_SEL}}", (c.modoApertura == 1) ? "selected" : "");
  
  html.replace("{{SENT_LEFT_SEL}}", (c.sentidoApertura == 0) ? "selected" : "");
  html.replace("{{SENT_RIGHT_SEL}}", (c.sentidoApertura == 1) ? "selected" : "");

  // Red (Usamos las variables dinámicas calculadas arriba)
  html.replace("{{CON_WIFI_SEL}}", (c.conexionRed == 0) ? "selected" : "");
  html.replace("{{CON_ETH_SEL}}", (c.conexionRed == 1) ? "selected" : "");
  html.replace("{{MODO_DHCP_SEL}}", (c.modoRed == 0) ? "selected" : "");
  html.replace("{{MODO_ESTATICA_SEL}}", (c.modoRed == 1) ? "selected" : "");
  
  html.replace("{{WIFI_SSID}}", htmlEscape(c.wifiSSID));
  html.replace("{{WIFI_PASS}}", htmlEscape(c.wifiPass));
  
  html.replace("{{IP}}", currentIP);
  html.replace("{{GATEWAY}}", currentGW);
  html.replace("{{SUBNET}}", currentMask);
  html.replace("{{DNS1}}", currentDNS1);
  html.replace("{{DNS2}}", currentDNS2);

  // API
  html.replace("{{URL_BASE}}", htmlEscape(c.urlBase));
  html.replace("{{URL_ACTUALIZA}}", htmlEscape(c.urlActualiza));

  html.replace("{{CFG_MSG}}", errorMessage_eth);
  errorMessage_eth = "";
  sendResponse(client, 200, "text/html; charset=utf-8", html);
}

void handleConfigSave(EthernetClient &client, const String &body)
{
  if (!requireAuth(client))
    return;

  TornoConfig c = cfgLoad();

  // Hardware
  c.deviceId = getParam(body, "deviceId");
  c.modoPasillo = (uint8_t)getParam(body, "modoPasillo").toInt();
  c.sentidoApertura = (uint8_t)getParam(body, "sentidoApertura").toInt();

  // Conexión y Red
  c.conexionRed = (uint8_t)getParam(body, "conexionRed").toInt();
  c.modoRed = (uint8_t)getParam(body, "modoRed").toInt();

  if (c.conexionRed == 0) { // Solo actualizar WiFi si estamos en ese modo o se envían datos
      String tmpSSID = getParam(body, "wifiSSID");
      if (tmpSSID.length() > 0) c.wifiSSID = tmpSSID;
      String tmpPass = getParam(body, "wifiPass");
      if (tmpPass.length() > 0) c.wifiPass = tmpPass;
  }

  c.ip   = getParam(body, "ip");
  c.gw   = getParam(body, "gw");
  c.mask = getParam(body, "mask");
  c.dns1 = getParam(body, "dns1");
  c.dns2 = getParam(body, "dns2");

  // APIs
  c.urlBase = getParam(body, "urlBase");
  c.urlActualiza = getParam(body, "urlActualiza");

  // Validación básica
  bool valid = isDeviceIdOK(c.deviceId) && c.urlBase.startsWith("http");
  
  if (c.modoRed == 1) { // Si es estática, validar IPs
      if (!isIPv4OK(c.ip) || !isIPv4OK(c.gw)) valid = false;
  }
  
  if (c.conexionRed == 0 && c.wifiSSID.length() < 1) {
      valid = false;
  }

  if (!valid)
  {
    errorMessage_eth = "<span style='color:red;'>Configuración inválida. Revisa los campos.</span>";
    sendResponse(client, 302, "text/plain", "", "Location: /config\r\n");
    return;
  }

  if (cfgSave(c))
  {
    redirectStatus(client, "success", "Guardado", "Reiniciando dispositivo...", "/menu", "Menú", "/menu", 5000);
    delay(500); // Un poco más de tiempo para asegurar el envío del response
    ESP.restart();
  }
}

void handleScanWiFi(EthernetClient &client) {
  // Opcional: Proteger también este endpoint si tienes autenticación
  if (!requireAuth(client)) return; 

  int n = WiFi.scanNetworks();
  String json = "[";
  
  for (int i = 0; i < n; ++i) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\", \"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  
  // Limpiamos la memoria del escaneo (muy importante en microcontroladores)
  WiFi.scanDelete(); 

  // Enviamos la respuesta. (Asegúrate de usar tu función equivalente para enviar cabeceras y body)
  sendResponse(client, 200, "application/json; charset=utf-8", json);
}

// ========================= Parámetros Técnicos (RS485) =========================
void handleParamsPage(EthernetClient &client)
{
  if (!requireAuth(client))
    return;
  if (modoApertura != 0)
  {
    sendResponse(client, 403, "text/plain", "Forbidden: Solo modo RS485");
    return;
  }
  sendFile(client, "/params.html");
}

void handleGetAllParams(EthernetClient &client)
{
  if (!requireAuth(client))
    return;
  TornoParams p = paramsLoad();

  String json = "{\"values\":[";
  json += String(p.machineId) + "," + String(p.openingMode) + "," + String(p.waitTime) + ",";
  json += String(p.voiceLeft) + "," + String(p.voiceRight) + "," + String(p.voiceVol) + ",";
  json += String(p.masterSpeed) + "," + String(p.slaveSpeed) + "," + String(p.debugMode) + ",";
  json += String(p.decelRange) + "," + String(p.selfTestSpeed) + "," + String(p.passageMode) + ",";
  json += String(p.closeControl) + "," + String(p.singleMotor) + "," + String(p.language) + ",";
  json += String(p.irRebound) + "," + String(p.pinchSens) + "," + String(p.reverseEntry) + ",";
  json += String(p.turnstileType) + "," + String(p.emergencyDir) + "," + String(p.motorResist) + ",";
  json += String(p.intrusionVoice) + "," + String(p.irDelay) + "," + String(p.motorDir) + ",";
  json += String(p.clutchLock) + "," + String(p.hallType) + "," + String(p.signalFilter) + ",";
  json += String(p.cardInside) + "," + String(p.tailgateAlarm) + "," + String(p.limitDev) + ",";
  json += String(p.pinchFree) + "," + String(p.memoryFree) + "," + String(p.slipMaster) + ",";
  json += String(p.slipSlave) + "," + String(p.irLogicMode) + "," + String(p.lightMaster) + ",";
  json += String(p.lightSlave);
  json += "]}";

  sendResponse(client, 200, "application/json", json);
}

void handleParamsSave(EthernetClient &client, const String &body)
{
  if (!requireAuth(client))
    return;
  TornoParams p = paramsLoad();
  int id = getParam(body, "id").toInt();
  int val = getParam(body, "val").toInt();

  switch (id)
  {
  case 0:
    p.machineId = val;
    break;
  case 1:
    p.openingMode = val;
    break;
  case 2:
    p.waitTime = val;
    break;
  case 3:
    p.voiceLeft = val;
    break;
  case 4:
    p.voiceRight = val;
    break;
  case 5:
    p.voiceVol = val;
    break;
  case 6:
    p.masterSpeed = val;
    break;
  case 7:
    p.slaveSpeed = val;
    break;
  case 8:
    p.debugMode = val;
    break;
  case 9:
    p.decelRange = val;
    break;
  case 10:
    p.selfTestSpeed = val;
    break;
  case 11:
    p.passageMode = val;
    break;
  case 12:
    p.closeControl = val;
    break;
  case 13:
    p.singleMotor = val;
    break;
  case 14:
    p.language = val;
    break;
  case 15:
    p.irRebound = val;
    break;
  case 16:
    p.pinchSens = val;
    break;
  case 17:
    p.reverseEntry = val;
    break;
  case 18:
    p.turnstileType = val;
    break;
  case 19:
    p.emergencyDir = val;
    break;
  case 20:
    p.motorResist = val;
    break;
  case 21:
    p.intrusionVoice = val;
    break;
  case 22:
    p.irDelay = val;
    break;
  case 23:
    p.motorDir = val;
    break;
  case 24:
    p.clutchLock = val;
    break;
  case 25:
    p.hallType = val;
    break;
  case 26:
    p.signalFilter = val;
    break;
  case 27:
    p.cardInside = val;
    break;
  case 28:
    p.tailgateAlarm = val;
    break;
  case 29:
    p.limitDev = val;
    break;
  case 30:
    p.pinchFree = val;
    break;
  case 31:
    p.memoryFree = val;
    break;
  case 32:
    p.slipMaster = val;
    break;
  case 33:
    p.slipSlave = val;
    break;
  case 34:
    p.irLogicMode = val;
    break;
  case 35:
    p.lightMaster = val;
    break;
  case 36:
    p.lightSlave = val;
    break;
  }

  paramsSave(p);
  paramsApplyToGlobals(p);
  if (RS485::setParam(MACHINE_ID, (uint8_t)id, (uint16_t)val))
  {
    delay(150);
    RS485::resetDevice(MACHINE_ID);
    sendResponse(client, 200, "application/json", "{\"status\":\"ok\"}");
  }
  else
  {
    sendResponse(client, 200, "application/json", "{\"status\":\"error\",\"msg\":\"Bus RS485 falló\"}");
  }
}

// ========================= Acciones y Estado =========================
void handleActionsPage(EthernetClient &client)
{
  if (!requireAuth(client))
    return;
  File file = LittleFS.open("/actions.html", "r");
  if (!file)
  {
    sendResponse(client, 404, "text/plain", "No encontrado");
    return;
  }
  String html = file.readString();
  file.close();
  html.replace("{{DEVICE_ID}}", htmlEscape(cfgLoad().deviceId));
  html.replace("{{RS485_CLASS}}", (modoApertura == 0) ? "" : "hidden");
  sendResponse(client, 200, "text/html; charset=utf-8", html);
}

void handleDoAction(EthernetClient &client, const String &fullPath)
{
  if (!requireAuth(client))
    return;

  String cmd = getQueryParam(fullPath, "cmd");
  int valP = getQueryParam(fullPath, "p").toInt();
  uint8_t numPeatones = (valP > 0) ? (uint8_t)valP : 1;
  uint8_t m = MACHINE_ID;

  if (modoApertura == 0)
  { // RS485
    if (cmd == "open_r")
    {
      if(debugSerie)
        Serial.printf("[ETH-SERVER] Apertura de entrada solicitada para %d peatones.\n", numPeatones);
      logbuf_pushf("[ETH-SERVER] Apertura de entrada solicitada para %d peatones.", numPeatones);
      if (sentidoApertura == 0)
        RS485::leftOpen(m, numPeatones);
      else
        RS485::rightOpen(m, numPeatones);
    }
    else if (cmd == "open_l")
    {
      Serial.printf("[ETH-SERVER] Apertura de salida solicitada para %d peatones.\n", numPeatones);
      logbuf_pushf("[ETH-SERVER] Apertura de salida solicitada para %d peatones.", numPeatones);
      if (sentidoApertura == 0)
        RS485::rightOpen(m, numPeatones);
      else
        RS485::leftOpen(m, numPeatones);
    }
    else if (cmd == "close")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Cierre de puertas solicitado.");
      logbuf_pushf("[ETH-SERVER] Cierre de puertas solicitado.");
      RS485::closeGate(m);
    }
    else if (cmd == "reset_r")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Reseteo de contador de entradas solicitado.");
      logbuf_pushf("[ETH-SERVER] Reseteo de contador de entradas solicitado.");

      if (sentidoApertura == 0)
        RS485::resetLeftCount(m);
      else
        RS485::resetRightCount(m);
    }
    else if (cmd == "reset_l")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Reseteo de contador de salidas solicitado.");
      logbuf_pushf("[ETH-SERVER] Reseteo de contador de salidas solicitado.");

      if (sentidoApertura == 0)
        RS485::resetRightCount(m);
      else
        RS485::resetLeftCount(m);
    }
    else if (cmd == "reset_full")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Reseteo del torno solicitado.");
      logbuf_pushf("[ETH-SERVER] Reseteo del torno solicitado.");
      RS485::resetDevice(m);
    }
    else if (cmd == "emergency")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] ¡EMERGENCIA ACTIVADA! Puertas abiertas.");
      logbuf_pushf("[ETH-SERVER] ¡EMERGENCIA ACTIVADA! Puertas abiertas.");
      // Enviamos comandos al RS485
      RS485::leftAlwaysOpen(m);
      delay(300); // Pequeña pausa para no saturar el bus RS485
      RS485::rightAlwaysOpen(m);

      logbuf_pushf("[SISTEMA] ¡EMERGENCIA ACTIVADA! Puertas abiertas.");
    }
    // --- RESTRICCIONES RS485 ---
    else if (cmd == "forbid_r")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Restricción de paso solicitada para la entrada.");
      logbuf_pushf("[ETH-SERVER] Restricción de paso solicitada para la entrada.");

      if (sentidoApertura == 0)
        RS485::forbiddenLeftPassage(m);
      else
        RS485::forbiddenRightPassage(m);
    }
    else if (cmd == "forbid_l")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Restricción de paso solicitada para la salida.");
      logbuf_pushf("[ETH-SERVER] Restricción de paso solicitada para la salida.");
      
      if (sentidoApertura == 0)
        RS485::forbiddenRightPassage(m);
      else
        RS485::forbiddenLeftPassage(m);
    }
    else if (cmd == "unrestrict")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Eliminación de restricciones de paso solicitada.");
      logbuf_pushf("[ETH-SERVER] Eliminación de restricciones de paso solicitada.");

      RS485::disablePassageRestriccion(m);
    }
  }
  else // MODO RELES
  {
    if (cmd == "open_r") // Entrada
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Apertura de entrada solicitada (modo relé).");
      logbuf_pushf("[ETH-SERVER] Apertura de entrada solicitada (modo relé).");

      (sentidoApertura == 0) ? rele::openEntry() : rele::openExit();
    }
    else if (cmd == "open_l") // Salida
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Apertura de salida solicitada (modo relé).");
      logbuf_pushf("[ETH-SERVER] Apertura de salida solicitada (modo relé).");
      (sentidoApertura == 0) ? rele::openExit() : rele::openEntry();
    }
    else if (cmd == "close")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Cierre de puertas solicitado (modo relé).");
      logbuf_pushf("[ETH-SERVER] Cierre de puertas solicitado (modo relé).");

      rele::close();
    }
    else if (cmd == "reset_r")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Reset de contadores de entrada solicitado.");
      logbuf_pushf("[ETH-SERVER] Reset de contadores de entrada solicitado.");
      (sentidoApertura == 0) ? salidasTotales = 0 : entradasTotales = 0;
    }
    else if (cmd == "reset_l")
    {
      if(debugSerie)
        Serial.println("[ETH-SERVER] Reset de contadores de salida solicitado.");
      logbuf_pushf("[ETH-SERVER] Reset de contadores de salida solicitado.");
      (sentidoApertura == 0) ? entradasTotales = 0 : salidasTotales = 0;
    }
  }
  sendResponse(client, 200, "application/json", "{\"status\":\"ok\"}");
}

void handleGetStatusJson(EthernetClient &client)
{
  if (!requireAuth(client))
    return;

  // Forzamos al bus RS485 a pedir el estado actual antes de responder a la web
  if (modoApertura == 0)
  {
    RS485::queryDeviceStatus(MACHINE_ID);
  }

  String json = "{";
  if (modoApertura == 0) // MODO RS485
  {
    RS485::StatusFrame st = RS485::getStatus();

    // Mapeo de contadores según sentidoApertura
    if (sentidoApertura == 0)
    {
      json += "\"cnt_der\":" + String(st.leftCount) + ",\"cnt_izq\":" + String(st.rightCount);
    }
    else
    {
      json += "\"cnt_der\":" + String(st.rightCount) + ",\"cnt_izq\":" + String(st.leftCount);
    }

    // Strings de estado procesados desde el StatusFrame
    json += ",\"gate_text\":\"" + String(st.gate == 0 ? "Cerrado" : (st.gate == 1 ? "Abierto Izq" : "Abierto Der")) + "\"";
    json += ",\"fault_text\":\"" + String(st.fault == 0 ? "OK" : "Fallo " + String(st.fault)) + "\"";
    json += ",\"alarm_text\":\"" + String(st.alarm == 0 ? "Ninguna" : "ALERTA " + String(st.alarm)) + "\"";
  }
  else // MODO RELES
  {
    if (sentidoApertura == 0)
    {
      json += "\"cnt_der\":" + String(entradasTotales) + ",\"cnt_izq\":" + String(salidasTotales);
    }
    else
    {
      json += "\"cnt_der\":" + String(salidasTotales) + ",\"cnt_izq\":" + String(entradasTotales);
    }
    json += ",\"gate_text\":\"Modo Relé\",\"fault_text\":\"N/A\",\"alarm_text\":\"N/A\"";
  }

  json += "}";
  sendResponse(client, 200, "application/json", json);
}

// ========================= Stats =========================
void handleStats(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 401, "application/json", "{\"ok\":false}");
    return;
  }
  lastActivityTime_eth = millis();

  // CORRECCIÓN: JSON corregido (comas y nombres de campo según HTML_CONFIG)
  String json = "{\"entradasTotales\":" + String(entradasTotales) +
                ",\"salidasTotales\":" + String(salidasTotales) +
                ",\"aperturasTotales\":" + String(entradasTotales + salidasTotales) + "}";

  sendResponse(client, 200, "application/json", json, "Cache-Control: no-store");
}

void handleResetAperturas(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 401, "application/json; charset=utf-8",
                 "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }

  lastActivityTime_eth = millis();

  // Reset contador
  entradasTotales = 0;
  salidasTotales = 0;

  // Respuesta OK
  sendResponse(client, 200, "application/json; charset=utf-8",
               "{\"ok\":true}",
               "Cache-Control: no-store\r\n");
}

// ========================= Logs page =========================
static void handleLogsPage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }
  lastActivityTime_eth = millis();
  sendFile(client, "/logs.html");
}

static void handleLogsData(EthernetClient &client, const String &fullPath)
{
  Serial.printf("[LOGS] registrado=%d enabled=%d\n", (int)registrado_eth, (int)logbuf_enabled());

  if (!registrado_eth)
  {
    sendResponse(client, 401, "application/json; charset=utf-8",
                 "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }

  lastActivityTime_eth = millis();

  uint32_t since = (uint32_t)getQueryParam(fullPath, "since").toInt();
  uint32_t next = 0;
  String json = logbuf_get_json_since(since, next);
  sendResponse(client, 200, "application/json; charset=utf-8", json);
}

// ========================= FS Upload pages =========================

void handleFsPage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }

  lastActivityTime_eth = millis();

  File file = LittleFS.open("/upload_fs.html", "r");
  if (!file)
  {
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /upload_fs.html en LittleFS.",
                   "/menu", "Volver al menú");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleFsUpload(EthernetClient &client, int contentLength, const String &contentType)
{
  if (debugSerie)
    Serial.println("handleFsUpload: inicio (subida de archivo suelto)");
  if (!client)
    return;

  if (!registrado_eth)
  {
    redirectStatus(client, "error", "Acceso denegado",
                   "Inicia sesión para actualizar el sistema de ficheros.",
                   "/", "Volver al login");
    return;
  }

  // boundary
  String boundary = "";
  int bpos = contentType.indexOf("boundary=");
  if (bpos >= 0)
  {
    boundary = contentType.substring(bpos + 9);
    boundary.trim();
  }
  if (boundary.length() == 0)
  {
    redirectStatus(client, "error", "Solicitud inválida",
                   "No se detectó 'boundary' en Content-Type. Reintenta la subida.",
                   "/upload_fs", "Volver a ficheros");
    return;
  }

  String boundaryMarker = "--" + boundary;
  int remaining = contentLength;

  // headers de la parte
  String partHeaders;
  while (remaining > 0 && client.connected())
  {
    char c = client.read();
    remaining--;
    partHeaders += c;
    if (partHeaders.endsWith("\r\n\r\n"))
      break;
  }

  // filename
  String filename;
  int fnPos = partHeaders.indexOf("filename=");
  if (fnPos >= 0)
  {
    int start = partHeaders.indexOf('"', fnPos);
    if (start >= 0)
    {
      int end = partHeaders.indexOf('"', start + 1);
      if (end > start)
        filename = partHeaders.substring(start + 1, end);
    }
  }

  // normalizar ruta
  int slash = filename.lastIndexOf('/');
  int bslash = filename.lastIndexOf('\\');
  int cut = max(slash, bslash);
  if (cut >= 0)
    filename = filename.substring(cut + 1);
  filename.trim();

  if (filename.length() == 0)
  {
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    redirectStatus(client, "error", "Archivo no válido",
                   "El nombre del archivo está vacío. Reintenta la subida.",
                   "/upload_fs", "Volver a ficheros");
    return;
  }

  // validar extensión permitida
  String lower = filename;
  lower.toLowerCase();
  if (!lower.endsWith(".html") &&
      !lower.endsWith(".png"))
  {
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    redirectStatus(client, "error", "Tipo de archivo no permitido",
                   "Solo se permiten: html, json, css, png, ico, jpg, jpeg, svg.",
                   "/upload_fs", "Volver a ficheros");
    return;
  }

  String path = "/" + filename;

  File f = LittleFS.open(path, "w");
  if (!f)
  {
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    redirectStatus(client, "error", "Error de almacenamiento",
                   "No se pudo escribir el archivo en LittleFS.",
                   "/upload_fs", "Volver a ficheros");
    return;
  }

  // escribir cuerpo detectando boundary
  String tail;
  const size_t BUF_SZ = 512;
  uint8_t buf[BUF_SZ];
  size_t totalWritten = 0;

  while (remaining > 0 && client.connected())
  {
    size_t toRead = remaining;
    if (toRead > BUF_SZ)
      toRead = BUF_SZ;
    int r = client.readBytes(buf, toRead);
    if (r <= 0)
      break;
    remaining -= r;

    String chunk;
    chunk.reserve(r);
    for (int i = 0; i < r; ++i)
      chunk += (char)buf[i];

    String combined = tail + chunk;
    int idxB = combined.indexOf(boundaryMarker);
    if (idxB >= 0)
    {
      int writeUpTo = idxB;
      if (writeUpTo >= 2 &&
          combined.charAt(writeUpTo - 2) == '\r' &&
          combined.charAt(writeUpTo - 1) == '\n')
      {
        writeUpTo -= 2;
      }

      for (int i = 0; i < writeUpTo; ++i)
        f.write((uint8_t)combined.charAt(i));
      totalWritten += writeUpTo;
      break;
    }
    else
    {
      int keep = boundaryMarker.length();
      if ((int)combined.length() <= keep)
      {
        tail = combined;
        continue;
      }

      int writeLen = combined.length() - keep;
      for (int i = 0; i < writeLen; ++i)
        f.write((uint8_t)combined.charAt(i));
      totalWritten += writeLen;
      tail = combined.substring(writeLen);
    }
  }

  f.close();

  if (totalWritten == 0)
  {
    redirectStatus(client, "error", "Subida incompleta",
                   "No se recibieron datos válidos para el fichero. Reintenta la subida.",
                   "/upload_fs", "Volver a ficheros");
    return;
  }

  // OK: página profesional + reinicio
  redirectStatus(client, "success", "Archivo actualizado",
                 "Archivo subido correctamente como " + path + ". El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  delay(150);
  ESP.restart();
}

// ========================= Bucle principal web =========================

void webHandleClient()
{
  EthernetClient client = serverETH.available();
  if (!client)
    return;

  if (debugSerie)
    Serial.println("Nuevo cliente conectado (Ethernet)");

  unsigned long start = millis();
  while (!client.available() && (millis() - start) < 500)
    delay(5);

  if (!client.available())
  {
    client.stop();
    if (debugSerie)
      Serial.println("Cliente sin datos, desconectado.");
    return;
  }

  String method, path, version;
  if (!readRequestLine(client, method, path, version))
  {
    mensajeError(client);
    client.stop();
    return;
  }

  // Guardar ruta completa con query para /status
  String fullPath = path;

  // Quitar query string para enrutado normal
  int q = path.indexOf('?');
  if (q >= 0)
    path = path.substring(0, q);

  if (debugSerie)
  {
    Serial.print("Request: ");
    Serial.print(method);
    Serial.print(" ");
    Serial.println(path);
  }

  String contentType;
  int contentLength = readHeaders(client, contentType);
  String body;

  // No leer body para uploads (se consume dentro)
  if (method == "POST" &&
      path != "/upload_firmware" &&
      path != "/upload_fs" &&
      contentLength > 0)
  {
    body = readBody(client, contentLength);
  }

  // Enrutado
  if (method == "GET" && path == "/")
    handleRoot(client);
  else if (method == "POST" && path == "/submit")
    handleSubmit(client, body);
  else if (method == "GET" && path == "/menu")
    handleMenu(client);
  else if (method == "GET" && path == "/reiniciar")
    handleReiniciarPage(client);
  else if (method == "POST" && path == "/reiniciar_dispositivo")
    handleReiniciarDispositivo(client);
  else if (method == "GET" && path == "/upload_firmware")
    handleFirmwarePage(client);
  else if (method == "POST" && path == "/upload_firmware")
    handleFirmwareUpload(client, contentLength, contentType);
  else if (method == "GET" && path == "/upload_fs")
    handleFsPage(client);
  else if (method == "POST" && path == "/upload_fs")
    handleFsUpload(client, contentLength, contentType);
  else if (method == "GET" && path == "/logout")
    handleLogout(client);
  else if (method == "GET" && path == "/config")
    handleConfigPage(client);
  else if (method == "POST" && path == "/config_save")
    handleConfigSave(client, body);
  // ---- NUEVAS RUTAS ----
  // else if (method == "GET" && path == "/params")
  //   handleParamsPage(client);
  // else if (method == "GET" && path == "/get_all_params")
  //   handleGetAllParams(client);
  // else if (method == "POST" && path == "/params_save")
  //   handleParamsSave(client, body);
  else if (method == "GET" && path == "/actions")
    handleActionsPage(client);
  else if (method == "GET" && path == "/do_action")
    handleDoAction(client, fullPath); // Ojo, usa fullPath para los params GET
  else if (method == "GET" && path == "/get_status_json")
    handleGetStatusJson(client);
  // ----------------------
  else if (method == "GET" && path == "/status")
    handleStatus(client, fullPath);
  else if (method == "GET" && path == "/logs")
    handleLogsPage(client);
  else if (method == "GET" && path == "/logs_data")
    handleLogsData(client, fullPath);
  // Manejo de estáticos con seguridad equiparable al onNotFound() de WiFi
  else if (method == "GET" && path != "/")
  {
    if (!registrado_eth)
    {
      if (!path.endsWith(".png") && !path.endsWith(".jpg") && !path.endsWith(".ico") && !path.endsWith(".css") && !path.endsWith(".js"))
      {
        sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
        return;
      }
    }
    if (LittleFS.exists(path))
      sendFile(client, path);
    else
      mensajeError(client);
  }
  else
  {
    mensajeError(client);
  }

  delay(20);
  client.stop();
  if (debugSerie)
    if (debugSerie)
      Serial.println("Cliente desconectado");
}
