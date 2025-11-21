// web.cpp
#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h> // Para futuras OTA si quieres
#include "definiciones.hpp"
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

  // Cabeceras
  if (extraHeaders.length() > 0)
  {
    client.print(extraHeaders);
    if (!extraHeaders.endsWith("\r\n"))
      client.print("\r\n");
  }

  // Content-Type (si hay cuerpo)
  if (body.length() > 0)
  {
    client.print("Content-Type: ");
    client.print(contentType);
    client.print("\r\n");
  }

  // Añadir Content-Length para conexión más fiable con navegadores
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
    // Escribir en bloques por si la conexión no acepta todo de una vez
    while (written < len && client.connected())
    {
      size_t chunk = len - written;
      if (chunk > 512)
        chunk = 512;
      client.write((const uint8_t *)(buf + written), chunk);
      written += chunk;
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

// Lee y descarta el resto de cabeceras, y en caso de POST, devuelve Content-Length
// Lee y descarta el resto de cabeceras, devuelve Content-Length y Content-Type (si existe)
static int readHeaders(EthernetClient &client, String &outContentType)
{
  int contentLength = 0;
  outContentType = "";

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
    {
      // Línea en blanco → fin de cabeceras
      break;
    }

    int colon = line.indexOf(':');
    if (colon < 0)
      continue;
    String headerName = line.substring(0, colon);
    headerName.trim();
    headerName.toLowerCase();

    String value = line.substring(colon + 1);
    value.trim();

    if (headerName == "content-length")
    {
      contentLength = value.toInt();
    }
    else if (headerName == "content-type")
    {
      outContentType = value;
    }
  }
  return contentLength;
}

// Lee el cuerpo de la petición según Content-Length
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

// ========================= Rutas =========================

static void mensajeError(EthernetClient &client)
{
  String mensaje = "<h1>404</h1>";
  mensaje += "Página no encontrada<br>";
  mensaje += "Intenta otra página<br>";
  sendResponse(client, 404, "text/html; charset=utf-8", mensaje);
}

void handleRoot(EthernetClient &client)
{
  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    if (debugSerie)
      Serial.println("handleRoot: index.html no encontrado en LittleFS");
    sendResponse(client, 404, "text/plain", "Archivo index.html no encontrado en LittleFS!");
    return;
  }

  size_t fsize = file.size();
  if (debugSerie)
  {
    Serial.print("handleRoot: index.html size=");
    Serial.println((unsigned long)fsize);
  }

  String htmlContent = file.readString();
  file.close();

  // Sustituimos marcador de error
  htmlContent.replace("pError", errorMessage_eth);
  errorMessage_eth = "";

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleSubmit(EthernetClient &client, const String &body)
{
  // body tipo: "password=XXXXXX&otroCampo=..."
  String pwd;
  int idx = body.indexOf("password=");
  if (idx >= 0)
  {
    int start = idx + 9;
    int end = body.indexOf('&', start);
    if (end < 0)
      end = body.length();
    pwd = body.substring(start, end);
    // Decodificar %xx y + si quieres, aquí va tal cual
  }

  if (pwd.length() == 0)
  {
    errorMessage_eth = "<span style=\"color:red;\">Por favor, introduzca una contraseña.</span>";
    // 302 a "/"
    String headers = "Location: /\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
    return;
  }

  if (pwd == credencialMaestra)
  {
    registrado_eth = true;
    lastActivityTime_eth = millis();
    errorMessage_eth = "";
    String headers = "Location: /menu\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
  }
  else
  {
    errorMessage_eth = "<span style=\"color:red;\">Contraseña incorrecta, reintente</span>";
    String headers = "Location: /\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
  }
}

void handleMenu(EthernetClient &client)
{
  if (!registrado_eth)
  {
    String headers = "Location: /\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
    return;
  }

  lastActivityTime_eth = millis();
  File file = LittleFS.open("/menu.html", "r");
  if (!file)
  {
    if (debugSerie)
      Serial.println("handleMenu: menu.html no encontrado en LittleFS");
    sendResponse(client, 404, "text/plain", "Archivo menu.html no encontrado en LittleFS!");
    return;
  }
  size_t fsize = file.size();
  if (debugSerie)
  {
    Serial.print("handleMenu: menu.html size=");
    Serial.println((unsigned long)fsize);
  }

  String htmlContent = file.readString();
  file.close();

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleReiniciarPage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    String headers = "Location: /\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
    return;
  }

  File file = LittleFS.open("/reiniciar.html", "r");
  if (!file)
  {
    sendResponse(client, 500, "text/plain", "Error interno del servidor: reiniciar.html no encontrado.");
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
    sendResponse(client, 401, "text/plain", "No autorizado.");
    return;
  }
  lastActivityTime_eth = millis();

  sendResponse(client, 200, "text/plain", "Reiniciando...");
  delay(100);
  ESP.restart();
}

void handleFirmwarePage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    String headers = "Location: /\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
    return;
  }
  lastActivityTime_eth = millis();

  File file = LittleFS.open("/upload.html", "r");
  if (!file)
  {
    sendResponse(client, 500, "text/plain", "Error interno: upload.html no encontrado.");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  // Reemplazar marcador VERSION_FIRMWARE con enVersion
  htmlContent.replace("VERSION_FIRMWARE", enVersion);

  sendResponse(client, 200, "text/html; charset=utf-8", htmlContent);
}

void handleLogout(EthernetClient &client)
{
  registrado_eth = false;
  errorMessage_eth = "";
  String headers = "Location: /\r\n";
  sendResponse(client, 302, "text/plain", "", headers);
}

// ========================= Upload firmware (multipart/form-data) =========================
// Guarda temporalmente el binario en LittleFS, valida el nombre y usa Update
void handleFirmwareUpload(EthernetClient &client, int contentLength, const String &contentType)
{
  if (debugSerie)
    Serial.println("handleFirmwareUpload: inicio");
  if (!client)
    return;

  // Extraer boundary de Content-Type
  String boundary = "";
  int bpos = contentType.indexOf("boundary=");
  if (bpos >= 0)
  {
    boundary = contentType.substring(bpos + 9);
    boundary.trim();
  }
  if (boundary.length() == 0)
  {
    sendResponse(client, 400, "text/plain", "Content-Type no multipart o sin boundary");
    return;
  }

  String boundaryMarker = "--" + boundary;
  if (debugSerie)
  {
    Serial.print("handleFirmwareUpload: boundary=");
    Serial.println(boundaryMarker);
  }

  int remaining = contentLength;

  // Leer hasta el final de las cabeceras de la parte (encabezados del form-data)
  String partHeaders;
  while (remaining > 0 && client.connected())
  {
    char c = client.read();
    remaining--;
    partHeaders += c;
    if (partHeaders.endsWith("\r\n\r\n"))
      break;
  }
  if (debugSerie)
  {
    Serial.print("handleFirmwareUpload: partHeaders=\n");
    Serial.println(partHeaders);
  }

  // Buscar filename en partHeaders
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

  if (debugSerie)
  {
    Serial.print("handleFirmwareUpload: raw filename=");
    Serial.println(filename);
  }

  // Quitar posibles rutas tipo C:\... o /home/...
  int slash = filename.lastIndexOf('/');
  int bslash = filename.lastIndexOf('\\');
  int cut = max(slash, bslash);
  if (cut >= 0)
  {
    filename = filename.substring(cut + 1);
  }
  filename.trim();

  if (debugSerie)
  {
    Serial.print("handleFirmwareUpload: normalized filename=");
    Serial.println(filename);
  }

  // Aquí mantenemos tu requisito actual: siempre "Elder.bin"
  String expected = "Elder.bin";
  expected.trim();

  if (filename != expected)
  {
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    String msg = "Nombre de fichero inválido. Debe ser: ";
    msg += expected;
    sendResponse(client, 400, "text/plain", msg);
    return;
  }

  // Archivo temporal en LittleFS
  const char *tmpPath = "/upload.bin";
  if (LittleFS.exists(tmpPath))
    LittleFS.remove(tmpPath);
  File f = LittleFS.open(tmpPath, "w");
  if (!f)
  {
    sendResponse(client, 500, "text/plain", "No se pudo crear archivo temporal en LittleFS");
    return;
  }

  // Leer remaining bytes y escribirlos en LittleFS, detectando boundary
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

    // Convertimos chunk a String para buscar boundary
    String chunk;
    chunk.reserve(r);
    for (int i = 0; i < r; ++i)
      chunk += (char)buf[i];

    String combined = tail + chunk;
    int idx = combined.indexOf(boundaryMarker);
    if (idx >= 0)
    {
      // boundary encontrado — escribir hasta antes del CRLF que precede al boundary
      int writeUpTo = idx;
      if (writeUpTo >= 2 &&
          combined.charAt(writeUpTo - 2) == '\r' &&
          combined.charAt(writeUpTo - 1) == '\n')
      {
        writeUpTo -= 2;
      }

      for (int i = 0; i < writeUpTo; ++i)
      {
        f.write((uint8_t)combined.charAt(i));
      }

      // Hemos terminado de recibir el fichero
      break;
    }
    else
    {
      // no se encontró boundary; escribir todo menos mantener cola del tamaño del boundaryMarker
      int keep = boundaryMarker.length();
      if ((int)combined.length() <= keep)
      {
        tail = combined;
        continue;
      }
      int writeLen = combined.length() - keep;
      for (int i = 0; i < writeLen; ++i)
      {
        f.write((uint8_t)combined.charAt(i));
      }
      tail = combined.substring(writeLen);
    }
  }

  f.close();
  if (debugSerie)
    Serial.print("handleFirmwareUpload: temporal escrito, size=");
  File f2 = LittleFS.open(tmpPath, "r");
  size_t fwSize = f2.size();
  if (debugSerie)
    Serial.println((unsigned long)fwSize);

  if (fwSize == 0)
  {
    f2.close();
    LittleFS.remove(tmpPath);
    sendResponse(client, 400, "text/plain", "Fichero vacío o error al recibir");
    return;
  }

  // Calcula espacio libre real para sketch (solo para COMPROBAR)
  size_t maxSketchSpace =
      (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000; // alineado a 4 KB

  if (debugSerie)
  {
    Serial.print("fwSize = ");
    Serial.println((unsigned long)fwSize);
    Serial.print("maxSketchSpace = ");
    Serial.println((unsigned long)maxSketchSpace);
  }

  if (fwSize > maxSketchSpace)
  {
    f2.close();
    LittleFS.remove(tmpPath);
    sendResponse(client, 400, "text/plain",
                 "Firmware demasiado grande para la partición OTA");
    return;
  }

  // ⚠️ IMPORTANTE: aquí usamos fwSize (como en tu fichero funcional)
  if (!Update.begin(fwSize))
  {
    if (debugSerie)
      Serial.printf("Update.begin(fwSize) failed: %s\n", Update.errorString());
    f2.close();
    LittleFS.remove(tmpPath);
    sendResponse(client, 500, "text/plain", "Update.begin falló");
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
        sendResponse(client, 500, "text/plain", "Error escribiendo firmware en flash");
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
      Serial.printf("Update error: %s\n", Update.errorString());
    sendResponse(client, 500, "text/plain", "Update falló al finalizar");
    return;
  }

  LittleFS.remove(tmpPath);

  // Responder y reiniciar
  sendResponse(client, 200, "text/plain", "Firmware cargado correctamente. Reiniciando...");
  delay(100);
  ESP.restart();
}

void handleFsPage(EthernetClient &client)
{
  if (!registrado_eth)
  {
    String headers = "Location: /\r\n";
    sendResponse(client, 302, "text/plain", "", headers);
    return;
  }

  lastActivityTime_eth = millis();

  File file = LittleFS.open("/ficheros.html", "r");
  if (!file)
  {
    sendResponse(client, 500, "text/plain", "Error interno: ficheros.html no encontrado.");
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
    sendResponse(client, 401, "text/plain", "No autorizado.");
    return;
  }

  // 1) Extraer boundary de Content-Type
  String boundary = "";
  int bpos = contentType.indexOf("boundary=");
  if (bpos >= 0)
  {
    boundary = contentType.substring(bpos + 9);
    boundary.trim();
  }
  if (boundary.length() == 0)
  {
    sendResponse(client, 400, "text/plain", "Content-Type no multipart o sin boundary");
    return;
  }

  String boundaryMarker = "--" + boundary;
  if (debugSerie)
  {
    Serial.print("handleFsUpload: boundary=");
    Serial.println(boundaryMarker);
  }

  int remaining = contentLength;

  // 2) Leer cabeceras de la parte (form-data)
  String partHeaders;
  while (remaining > 0 && client.connected())
  {
    char c = client.read();
    remaining--;
    partHeaders += c;
    if (partHeaders.endsWith("\r\n\r\n"))
    {
      break; // fin de cabeceras de la parte
    }
  }
  if (debugSerie)
  {
    Serial.print("handleFsUpload: partHeaders=\n");
    Serial.println(partHeaders);
  }

  // 3) Obtener filename del form-data
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
  if (debugSerie)
  {
    Serial.print("handleFsUpload: raw filename=");
    Serial.println(filename);
  }

  // 4) Normalizar: quitar ruta (C:\... o /home/...)
  int slash = filename.lastIndexOf('/');
  int bslash = filename.lastIndexOf('\\');
  int cut = max(slash, bslash);
  if (cut >= 0)
  {
    filename = filename.substring(cut + 1);
  }
  filename.trim();

  if (debugSerie)
  {
    Serial.print("handleFsUpload: normalized filename=");
    Serial.println(filename);
  }

  // 5) Validar extensión (.html o .json)
  if (filename.length() == 0)
  {
    sendResponse(client, 400, "text/plain", "Nombre de fichero vacío");
    // consumir el resto del body
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    return;
  }

  String lower = filename;
  lower.toLowerCase();
  if (!lower.endsWith(".html") && !lower.endsWith(".json"))
  {
    // Consumir resto del body para limpiar el request
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    sendResponse(client, 400, "text/plain", "Solo se permiten ficheros .html o .json");
    return;
  }

  // 6) Abrir archivo en LittleFS: se sobreescribe si existe
  String path = "/" + filename;
  if (debugSerie)
  {
    Serial.print("handleFsUpload: escribiendo en ");
    Serial.println(path);
  }

  File f = LittleFS.open(path, "w");
  if (!f)
  {
    // consumir resto del body
    while (remaining > 0 && client.connected())
    {
      client.read();
      remaining--;
    }
    sendResponse(client, 500, "text/plain", "No se pudo abrir el fichero en LittleFS");
    return;
  }

  // 7) Leer el cuerpo de la parte y escribir al fichero,
  //    detectando el boundary final (igual filosofía que en firmware)
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

    // Convertimos el chunk a String para buscar el boundary
    String chunk;
    chunk.reserve(r);
    for (int i = 0; i < r; ++i)
      chunk += (char)buf[i];

    String combined = tail + chunk;
    int idx = combined.indexOf(boundaryMarker);
    if (idx >= 0)
    {
      // Hemos encontrado el boundary: datos válidos hasta idx (menos CRLF)
      int writeUpTo = idx;
      if (writeUpTo >= 2 &&
          combined.charAt(writeUpTo - 2) == '\r' &&
          combined.charAt(writeUpTo - 1) == '\n')
      {
        writeUpTo -= 2;
      }

      // Escribir esa parte al fichero
      for (int i = 0; i < writeUpTo; ++i)
      {
        f.write((uint8_t)combined.charAt(i));
      }
      totalWritten += writeUpTo;
      // Hemos terminado
      break;
    }
    else
    {
      // No hemos encontrado boundary, mantenemos una cola del tamaño del boundary
      int keep = boundaryMarker.length();
      if ((int)combined.length() <= keep)
      {
        tail = combined;
        continue;
      }

      int writeLen = combined.length() - keep;
      // Escribir writeLen bytes al fichero
      for (int i = 0; i < writeLen; ++i)
      {
        f.write((uint8_t)combined.charAt(i));
      }
      totalWritten += writeLen;
      // Guardamos la cola para el siguiente loop
      tail = combined.substring(writeLen);
    }
  }

  f.close();

  if (debugSerie)
  {
    Serial.print("handleFsUpload: total escrito en LittleFS=");
    Serial.println((unsigned long)totalWritten);
  }

  if (totalWritten == 0)
  {
    sendResponse(client, 400, "text/plain", "No se recibieron datos válidos para el fichero");
    return;
  }

  // 8) Respuesta OK
  String msg = "Fichero subido correctamente como ";
  msg += path;
  sendResponse(client, 200, "text/plain", msg);
  // Reiniciar para aplicar cambios en LittleFS
  delay(100);
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
  while (!client.available() && (millis() - start) < 2000)
  {
    delay(1);
  }
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

  // ⚠️ No leer el body para /upload_firmware ni /upload_fs
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
  else
  {
    mensajeError(client);
  }

  delay(1);
  client.stop();
  if (debugSerie)
    Serial.println("Cliente desconectado");
}
