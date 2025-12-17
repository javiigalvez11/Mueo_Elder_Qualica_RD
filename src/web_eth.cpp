// web_eth.cpp
#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h>

#include "definiciones.hpp"
#include "web_eth.hpp"
#include "config_prefs.hpp"
#include "logBuf.hpp"

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
      if (chunk > 512)
        chunk = 512;

      int w = client.write((const uint8_t *)(buf + written), chunk);
      if (w <= 0)
        break;

      written += (size_t)w;

      delay(0); // o vTaskDelay(pdMS_TO_TICKS(1));
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

static String urlDecode(const String &s)
{
  String out;
  out.reserve(s.length());

  for (int i = 0; i < (int)s.length(); i++)
  {
    char c = s[i];
    if (c == '+')
      out += ' ';
    else if (c == '%' && i + 2 < (int)s.length())
    {
      auto hex = [](char x) -> int
      {
        if (x >= '0' && x <= '9')
          return x - '0';
        if (x >= 'a' && x <= 'f')
          return x - 'a' + 10;
        if (x >= 'A' && x <= 'F')
          return x - 'A' + 10;
        return -1;
      };

      int a = hex(s[i + 1]), b = hex(s[i + 2]);
      if (a >= 0 && b >= 0)
      {
        out += char((a << 4) | b);
        i += 2;
      }
      else
        out += c;
    }
    else
      out += c;
  }
  return out;
}

// Encode para meter mensajes en querystring de /status
static String urlEncode(const String &s)
{
  const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(s.length() * 2);

  for (int i = 0; i < (int)s.length(); i++)
  {
    uint8_t c = (uint8_t)s[i];

    // seguros
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~')
    {
      out += (char)c;
    }
    else if (c == ' ')
    {
      out += '+';
    }
    else
    {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
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

static String htmlEscape(String s)
{
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
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

static bool isDeviceIdOK(const String &s)
{
  if (s.length() != 5){
    if (debugSerie)
    {
      Serial.println("Tamaño de deviceId: " + String(s.length()));
      Serial.println("isDeviceIdOK: longitud incorrecta");
    }

    logbuf_pushf(("Tamaño de deviceId: " + String(s.length())).c_str());
    logbuf_pushf("isDeviceIdOK: longitud incorrecta");
    return false;
  }
  if (!(s[0] == 'M' && s[1] == 'E')){
    if (debugSerie)
    {
      Serial.println("isDeviceIdOK: prefijo incorrecto");
    }
    logbuf_pushf("isDeviceIdOK: prefijo incorrecto");
    return false;
  }
  for (int i = 2; i < 5; i++)
  {
    if (s[i] < '0' || s[i] > '9')
      return false;
  }
  if(debugSerie)
  {
    Serial.println("isDeviceIdOK: OK");
  } 
  logbuf_pushf("isDeviceIdOK: OK");
  return true;

}

static bool isIPv4OK(const String &ip)
{
  int parts = 0;
  int start = 0;

  for (int i = 0; i <= (int)ip.length(); i++)
  {
    if (i == (int)ip.length() || ip[i] == '.')
    {
      if (i == start) return false;      // octeto vacío
      if (i - start > 3) return false;   // más de 3 dígitos

      int val = 0;
      for (int j = start; j < i; j++)
      {
        char c = ip[j];
        if (c < '0' || c > '9') return false;
        val = val * 10 + (c - '0');
      }

      if (val < 0 || val > 255)
      {
        if (debugSerie)
          Serial.println("isIPv4OK: octeto fuera de rango: " + String(val));
        logbuf_pushf(("isIPv4OK: octeto fuera de rango: " + String(val)).c_str());
        return false;
      }

      parts++;
      start = i + 1;
    }
  }

  if (debugSerie) Serial.println("isIPv4OK: OK");
  return (parts == 4);
}

static bool isUrlBaseOK(const String &u)
{
  if (u.length() == 0 || u.length() > 100) return false;
  if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
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

static String getQueryParam(const String &pathWithQuery, const String &key)
{
  int q = pathWithQuery.indexOf('?');
  if (q < 0)
    return "";

  String query = pathWithQuery.substring(q + 1);
  String needle = key + "=";

  int p = query.indexOf(needle);
  if (p < 0)
    return "";

  int start = p + needle.length();
  int end = query.indexOf('&', start);
  if (end < 0)
    end = query.length();

  return urlDecode(query.substring(start, end));
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

  htmlContent.replace("pError", errorMessage_eth);
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
  if (!registrado_eth)
  {
    sendResponse(client, 302, "text/plain", "", "Location: /\r\n");
    return;
  }

  lastActivityTime_eth = millis();

  File file = LittleFS.open("/config.html", "r");
  if (!file)
  {
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /config.html en LittleFS.",
                   "/menu", "Volver al menú");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  TornoConfig c = cfgLoad();

  htmlContent.replace("{{DEVICE_ID}}", htmlEscape(c.deviceId));
  htmlContent.replace("{{SENTIDO_ENTRADA_SEL}}", (c.sentido == "Entrada") ? "selected" : "");
  htmlContent.replace("{{SENTIDO_SALIDA_SEL}}", (c.sentido == "Salida") ? "selected" : "");

  htmlContent.replace("{{MODO_DHCP_SEL}}", (c.modoRed == 0) ? "selected" : "");
  htmlContent.replace("{{MODO_ESTATICA_SEL}}", (c.modoRed == 1) ? "selected" : "");

  htmlContent.replace("{{IP}}", htmlEscape(c.ip));
  htmlContent.replace("{{GATEWAY}}", htmlEscape(c.gw));
  htmlContent.replace("{{SUBNET}}", htmlEscape(c.mask));
  htmlContent.replace("{{DNS1}}", htmlEscape(c.dns1));
  htmlContent.replace("{{DNS2}}", htmlEscape(c.dns2));
  htmlContent.replace("{{URLBASE}}", htmlEscape(c.urlBase));

  htmlContent.replace("{{CFG_MSG}}", errorMessage_eth);
  errorMessage_eth = "";

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleConfigSave(EthernetClient &client, const String &body)
{
  if (!registrado_eth)
  {
    redirectStatus(client, "error", "Acceso denegado",
                   "Inicia sesión para guardar la configuración.",
                   "/", "Volver al login");
    return;
  }

  lastActivityTime_eth = millis();

  TornoConfig c;
  c.deviceId = getParam(body, "deviceId");
  c.sentido = getParam(body, "sentido");
  c.modoRed = (uint8_t)getParam(body, "modoRed").toInt();

  c.ip = getParam(body, "ip");
  c.gw = getParam(body, "gw");
  c.mask = getParam(body, "mask");
  c.dns1 = getParam(body, "dns1");
  c.dns2 = getParam(body, "dns2");

  c.urlBase = getParam(body, "urlBase");

  c.deviceId.toUpperCase(); // por si acaso

  if (!isDeviceIdOK(c.deviceId) ||
      !isIPv4OK(c.ip) ||
      !isIPv4OK(c.gw) ||
      !isIPv4OK(c.mask) ||
      !isIPv4OK(c.dns1) ||
      !isIPv4OK(c.dns2) ||
      !isUrlBaseOK(c.urlBase))
  {
    errorMessage_eth = "<span style='color:red;'>Config inválida. Revisa: deviceId (ME######), IPv4 (0-255) y URL (100).</span>";
    sendResponse(client, 302, "text/plain", "", "Location: /config\r\n");
    return;
  }

  logbuf_pushf("Guardando configuración:");
  logbuf_pushf((" deviceId=" + c.deviceId).c_str());
  logbuf_pushf((" sentido=" + c.sentido).c_str());
  logbuf_pushf((" modoRed=" + String(c.modoRed)).c_str());
  logbuf_pushf((" ip=" + c.ip).c_str());
  logbuf_pushf((" gw=" + c.gw).c_str());
  logbuf_pushf((" mask=" + c.mask).c_str());
  logbuf_pushf((" dns1=" + c.dns1).c_str());
  logbuf_pushf((" dns2=" + c.dns2).c_str());

  if (!cfgSave(c))
  {
    // Mantengo tu flujo de mensaje dentro del propio config.html (bonito)
    errorMessage_eth = "<span style='color:red;'>Config inválida. Revisa campos (IPs / URL / nombre / modo).</span>";
    sendResponse(client, 302, "text/plain", "", "Location: /config\r\n");
    return;
  }

  // OK: página profesional + reinicio
  redirectStatus(client, "success", "Configuración guardada",
                 "Configuración guardada correctamente. El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  delay(150);
  ESP.restart();
}

// ========================= Stats =========================
void handleStats(EthernetClient &client)
{
  if (!registrado_eth)
  {
    sendResponse(client, 401, "application/json; charset=utf-8",
                 "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }

  // Refresca sesión
  lastActivityTime_eth = millis();

  // JSON mínimo
  String json;
  json.reserve(64);
  json += "{\"aperturasTotales\":";
  json += String((uint32_t)aperturasTotales);
  json += "}";

  sendResponse(client, 200, "application/json; charset=utf-8", json,
               "Cache-Control: no-store\r\n");
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
  aperturasTotales = 0;

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

  File file = LittleFS.open("/ficheros.html", "r");
  if (!file)
  {
    redirectStatus(client, "error", "Sistema incompleto",
                   "No se encuentra /ficheros.html en LittleFS.",
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
    delay(1);

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
  {
    handleRoot(client);
  }
  else if (method == "POST" && path == "/submit")
  {
    handleSubmit(client, body);
  }
  else if (method == "GET" && path == "/menu")
  {
    handleMenu(client);
  }
  else if (method == "GET" && path == "/reiniciar")
  {
    handleReiniciarPage(client);
  }
  else if (method == "POST" && path == "/reiniciar_dispositivo")
  {
    handleReiniciarDispositivo(client);
  }
  else if (method == "GET" && path == "/upload_firmware")
  {
    handleFirmwarePage(client);
  }
  else if (method == "POST" && path == "/upload_firmware")
  {
    handleFirmwareUpload(client, contentLength, contentType);
  }
  else if (method == "GET" && path == "/upload_fs")
  {
    handleFsPage(client);
  }
  else if (method == "POST" && path == "/upload_fs")
  {
    handleFsUpload(client, contentLength, contentType);
  }
  else if (method == "GET" && path == "/logout")
  {
    handleLogout(client);
  }
  else if (method == "GET" && path == "/config")
  {
    handleConfigPage(client);
  }
  else if (method == "POST" && path == "/config_save")
  {
    handleConfigSave(client, body);
  }
  else if (method == "GET" && path == "/stats")
  {
    handleStats(client);
  }
  else if (method == "POST" && path == "/reset_aperturas")
  {
    handleResetAperturas(client);
  }
  else if (method == "GET" && path == "/status")
  {
    // IMPORTANTE: aquí usamos la ruta completa con query
    handleStatus(client, fullPath);
  }
  else if (method == "GET" && path == "/logs")
  {
    handleLogsPage(client);
  }
  else if (method == "GET" && path == "/logs_data")
  {
    handleLogsData(client, fullPath);
  }
  else if (method == "GET" && path != "/" && LittleFS.exists(path))
  {
    // Estáticos (logo, css, etc.)
    sendFile(client, path);
  }
  else
  {
    mensajeError(client);
  }

  vTaskDelay(pdMS_TO_TICKS(1));
  client.stop();
  if (debugSerie)
    if (debugSerie) Serial.println("Cliente desconectado");
}
