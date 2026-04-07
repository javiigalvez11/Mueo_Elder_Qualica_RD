// web_ap.cpp
#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h>
#include <WebServer.h>

#include "web_ap.hpp"
#include "definiciones.hpp"
#include "config_prefs.hpp"
#include "logBuf.hpp"

// ======= ESTADO GLOBAL PARA OTA POR AP =======
static bool otaUploadOk_ap = false;
static String otaErrorMsg_ap = "";

// ======= ESTADO GLOBAL PARA FS UPLOAD POR AP =======
static File fsUploadFile_ap;
static bool fsUploadOk_ap = false;
static String fsUploadErr_ap = "";
static String fsUploadPath_ap = "";
static size_t fsUploadTotal_ap = 0;

// ========================= Helpers (HTML/Status/Files) =========================

static String htmlEscape(String s)
{
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
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
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~')
      out += (char)c;
    else if (c == ' ')
      out += '+';
    else
    {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
}

static void redirectStatus(const String &kind,
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

  serverAP.sendHeader("Location", loc);
  serverAP.send(302, "text/plain", "");
}

static String contentTypeFromPath(const String &path)
{
  if (path.endsWith(".html")) return "text/html; charset=utf-8";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg"))  return "image/jpeg";
  if (path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".svg"))  return "image/svg+xml";
  if (path.endsWith(".ico"))  return "image/x-icon";
  if (path.endsWith(".json")) return "application/json";
  return "application/octet-stream";
}

static void sendFileAP(const String &path)
{
  File file = LittleFS.open(path, "r");
  if (!file)
  {
    redirectStatus("error", "Recurso no disponible",
                   "El archivo solicitado no existe en el dispositivo.",
                   "/menu", "Volver al menú");
    return;
  }

  String ct = contentTypeFromPath(path);
  serverAP.streamFile(file, ct);
  file.close();
}

static inline bool requireAuthAP()
{
  if (!registrado_ap)
  {
    serverAP.sendHeader("Location", "/");
    serverAP.send(302, "text/plain", "");
    return false;
  }
  lastActivityTime_ap = millis();
  return true;
}

// ========================= Validaciones =========================

static bool isDeviceIdOK(const String &s)
{
  if (s.length() != 5) return false;
  if (!(s[0] == 'M' && s[1] == 'E')) return false;
  for (int i = 2; i < 5; i++)
    if (s[i] < '0' || s[i] > '9') return false;
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
      if (i == start) return false;
      if (i - start > 3) return false;

      int val = 0;
      for (int j = start; j < i; j++)
      {
        char c = ip[j];
        if (c < '0' || c > '9') return false;
        val = val * 10 + (c - '0');
      }
      if (val < 0 || val > 255) return false;

      parts++;
      start = i + 1;
    }
  }
  return (parts == 4);
}

static bool isUrlBaseOK(const String &u)
{
  // LÍMITE: 100 (tal como pediste)
  if (u.length() == 0 || u.length() > 100) return false;
  if (!(u.startsWith("http://") || u.startsWith("https://"))) return false;
  return true;
}

// ========================= Handlers base =========================

static void handleStatus()
{
  File file = LittleFS.open("/status.html", "r");
  if (!file)
  {
    serverAP.send(500, "text/plain", "status.html no encontrado en LittleFS");
    return;
  }

  String html = file.readString();
  file.close();

  String kind = serverAP.hasArg("kind") ? serverAP.arg("kind") : "info";
  if (kind != "success" && kind != "error" && kind != "info") kind = "info";

  String title = serverAP.hasArg("title") ? serverAP.arg("title") : "";
  if (title.length() == 0)
    title = (kind == "success") ? "Éxito" : (kind == "error") ? "Error" : "Información";

  String msg = serverAP.hasArg("msg") ? serverAP.arg("msg") : "Operación completada.";

  String back = serverAP.hasArg("back") ? serverAP.arg("back") : "/menu";
  String backText = serverAP.hasArg("backText") ? serverAP.arg("backText") : "Volver";

  String redir = serverAP.hasArg("redir") ? serverAP.arg("redir") : "";
  String ms    = serverAP.hasArg("ms")    ? serverAP.arg("ms")    : "";

  html.replace("{{KIND}}", htmlEscape(kind));
  html.replace("{{TITLE}}", htmlEscape(title));
  html.replace("{{MESSAGE}}", htmlEscape(msg));
  html.replace("{{BACK_URL}}", htmlEscape(back));
  html.replace("{{BACK_TEXT}}", htmlEscape(backText));
  html.replace("{{AUTO_REDIRECT_URL}}", htmlEscape(redir));
  html.replace("{{AUTO_REDIRECT_MS}}", htmlEscape(ms));

  if (redir.length() == 0 || ms.length() == 0)
    html.replace("{{REDIRECT_HINT}}", "");
  else
    html.replace("{{REDIRECT_HINT}}", "Redirigiendo automáticamente...");

  serverAP.send(200, "text/html; charset=utf-8", html);
}

static void handleRoot()
{
  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    redirectStatus("error", "Sistema incompleto",
                   "No se encuentra /index.html en LittleFS. Sube el sistema de ficheros.",
                   "/upload_fs", "Ir a actualizar FS");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  htmlContent.replace("pError", errorMessage_ap);
  errorMessage_ap = "";

  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
}

static void handleSubmit()
{
  String pwd = serverAP.hasArg("password") ? serverAP.arg("password") : "";

  if (pwd.length() == 0)
  {
    errorMessage_ap = "<span style=\"color:red;\">Por favor, introduzca una contraseña.</span>";
    serverAP.sendHeader("Location", "/");
    serverAP.send(302, "text/plain", "");
    return;
  }

  if (pwd == credencialMaestra)
  {
    registrado_ap = true;
    lastActivityTime_ap = millis();
    errorMessage_ap = "";

    logbuf_clear();
    logbuf_enable(true);

    serverAP.sendHeader("Location", "/menu");
    serverAP.send(302, "text/plain", "");
  }
  else
  {
    errorMessage_ap = "<span style=\"color:red;\">Contraseña incorrecta, reintente.</span>";
    serverAP.sendHeader("Location", "/");
    serverAP.send(302, "text/plain", "");
  }
}

static void handleMenu()
{
  if (!requireAuthAP()) return;

  File file = LittleFS.open("/menu.html", "r");
  if (!file)
  {
    redirectStatus("error", "Sistema incompleto",
                   "No se encuentra /menu.html en LittleFS. Sube el sistema de ficheros.",
                   "/upload_fs", "Ir a actualizar FS");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
}

static void handleReiniciarPage()
{
  if (!requireAuthAP()) return;

  File file = LittleFS.open("/reiniciar.html", "r");
  if (!file)
  {
    redirectStatus("error", "Sistema incompleto",
                   "No se encuentra /reiniciar.html en LittleFS.",
                   "/menu", "Volver al menú");
    return;
  }

  serverAP.streamFile(file, "text/html; charset=utf-8");
  file.close();
}

static void handleReiniciarDispositivo()
{
  if (!requireAuthAP())
  {
    redirectStatus("error", "Acceso denegado",
                   "No tienes sesión iniciada para reiniciar el dispositivo.",
                   "/", "Volver al login");
    return;
  }

  redirectStatus("info", "Reinicio",
                 "Se ha solicitado el reinicio. El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  delay(150);
  ESP.restart();
}

static void handleFirmwarePage()
{
  if (!requireAuthAP()) return;

  File file = LittleFS.open("/upload.html", "r");
  if (!file)
  {
    redirectStatus("error", "Sistema incompleto",
                   "No se encuentra /upload.html en LittleFS.",
                   "/menu", "Volver al menú");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  htmlContent.replace("VERSION_FIRMWARE", enVersion);
  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
}

static void handleLogout()
{
  registrado_ap = false;
  errorMessage_ap = "";

  logbuf_enable(false);
  logbuf_clear();

  serverAP.sendHeader("Location", "/");
  serverAP.send(302, "text/plain", "");
}

// ========================= OTA Firmware (AP) =========================

static void handleFirmwareUpload()
{
  if (!registrado_ap)
  {
    otaErrorMsg_ap = "No autorizado";
    return;
  }

  HTTPUpload &upload = serverAP.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    otaUploadOk_ap = false;
    otaErrorMsg_ap = "";

    String filename = upload.filename;
    int slash = filename.lastIndexOf('/');
    int bslash = filename.lastIndexOf('\\');
    int cut = max(slash, bslash);
    if (cut >= 0) filename = filename.substring(cut + 1);
    filename.trim();

    // Validación: debe ser DEVICE_ID.bin (igual que ethernet)
    String expected = DEVICE_ID + ".bin";
    expected.trim();

    if (filename != expected)
    {
      otaErrorMsg_ap = "El fichero debe llamarse exactamente: " + expected;
      return;
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      otaErrorMsg_ap = "Update.begin() falló al iniciar la OTA";
      return;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (otaErrorMsg_ap.length() == 0)
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        otaErrorMsg_ap = "Error escribiendo firmware en flash";
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (otaErrorMsg_ap.length() == 0)
    {
      if (Update.end(true))
        otaUploadOk_ap = true;
      else
        otaErrorMsg_ap = "Update.end() falló al finalizar la OTA";
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    otaErrorMsg_ap = "Carga OTA abortada por el cliente";
    Update.end();
  }
}

static void handleFirmwareUploadComplete()
{
  if (!registrado_ap)
  {
    redirectStatus("error", "Acceso denegado",
                   "Inicia sesión para poder actualizar el firmware.",
                   "/", "Volver al login");
    return;
  }

  if (otaErrorMsg_ap.length() > 0)
  {
    redirectStatus("error", "Actualización fallida",
                   otaErrorMsg_ap,
                   "/upload_firmware", "Volver a firmware");
    otaUploadOk_ap = false;
    otaErrorMsg_ap = "";
    return;
  }

  if (!otaUploadOk_ap || Update.hasError())
  {
    redirectStatus("error", "Actualización fallida",
                   "No se pudo completar la actualización correctamente.",
                   "/upload_firmware", "Volver a firmware");
    otaUploadOk_ap = false;
    otaErrorMsg_ap = "";
    return;
  }

  redirectStatus("success", "Firmware actualizado",
                 "Firmware cargado correctamente. El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  otaUploadOk_ap = false;
  otaErrorMsg_ap = "";
  delay(150);
  ESP.restart();
}

// ========================= Config (Preferences) =========================

static void handleConfigPage()
{
  if (!requireAuthAP()) return;

  File file = LittleFS.open("/config.html", "r");
  if (!file)
  {
    redirectStatus("error", "Sistema incompleto",
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

  htmlContent.replace("{{CFG_MSG}}", errorMessage_ap);
  errorMessage_ap = "";

  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
}

static void handleConfigSave()
{
  if (!requireAuthAP())
  {
    redirectStatus("error", "Acceso denegado",
                   "Inicia sesión para guardar la configuración.",
                   "/", "Volver al login");
    return;
  }

  TornoConfig c;
  c.deviceId = serverAP.hasArg("deviceId") ? serverAP.arg("deviceId") : "";
  c.sentido  = serverAP.hasArg("sentido")  ? serverAP.arg("sentido")  : "";
  c.modoRed  = (uint8_t)(serverAP.hasArg("modoRed") ? serverAP.arg("modoRed").toInt() : 0);

  c.ip   = serverAP.hasArg("ip")   ? serverAP.arg("ip")   : "";
  c.gw   = serverAP.hasArg("gw")   ? serverAP.arg("gw")   : "";
  c.mask = serverAP.hasArg("mask") ? serverAP.arg("mask") : "";
  c.dns1 = serverAP.hasArg("dns1") ? serverAP.arg("dns1") : "";
  c.dns2 = serverAP.hasArg("dns2") ? serverAP.arg("dns2") : "";

  c.urlBase = serverAP.hasArg("urlBase") ? serverAP.arg("urlBase") : "";

  c.deviceId.toUpperCase();

  if (!isDeviceIdOK(c.deviceId) ||
      !isIPv4OK(c.ip) ||
      !isIPv4OK(c.gw) ||
      !isIPv4OK(c.mask) ||
      !isIPv4OK(c.dns1) ||
      !isIPv4OK(c.dns2) ||
      !isUrlBaseOK(c.urlBase))
  {
    // Nota: aquí ya queda correcto el mensaje con el límite 100
    errorMessage_ap = "<span style='color:red;'>Config inválida. Revisa: deviceId (ME###), IPv4 (0-255) y URL (http(s)://, ≤100).</span>";
    serverAP.sendHeader("Location", "/config");
    serverAP.send(302, "text/plain", "");
    return;
  }

  logbuf_pushf("Guardando configuración (AP):");
  logbuf_pushf((" deviceId=" + c.deviceId).c_str());
  logbuf_pushf((" sentido=" + c.sentido).c_str());
  logbuf_pushf((" modoRed=" + String(c.modoRed)).c_str());
  logbuf_pushf((" ip=" + c.ip).c_str());
  logbuf_pushf((" gw=" + c.gw).c_str());
  logbuf_pushf((" mask=" + c.mask).c_str());
  logbuf_pushf((" dns1=" + c.dns1).c_str());
  logbuf_pushf((" dns2=" + c.dns2).c_str());
  logbuf_pushf((" urlBase=" + c.urlBase).c_str());

  if (!cfgSave(c))
  {
    errorMessage_ap = "<span style='color:red;'>No se pudo guardar la configuración.</span>";
    serverAP.sendHeader("Location", "/config");
    serverAP.send(302, "text/plain", "");
    return;
  }

  redirectStatus("success", "Configuración guardada",
                 "Configuración guardada correctamente. El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  delay(150);
  ESP.restart();
}

// ========================= Stats =========================

static void handleStats()
{
  if (!registrado_ap)
  {
    serverAP.send(401, "application/json; charset=utf-8",
                  "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }
  lastActivityTime_ap = millis();

  String json;
  json.reserve(64);
  json += "{\"aperturasTotales\":";
  json += String((uint32_t)aperturasTotales);
  json += "}";

  serverAP.sendHeader("Cache-Control", "no-store");
  serverAP.send(200, "application/json; charset=utf-8", json);
}

static void handleResetAperturas()
{
  if (!registrado_ap)
  {
    serverAP.send(401, "application/json; charset=utf-8",
                  "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }
  lastActivityTime_ap = millis();

  aperturasTotales = 0;

  serverAP.sendHeader("Cache-Control", "no-store");
  serverAP.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

// ========================= Logs =========================

static void handleLogsPage()
{
  if (!requireAuthAP()) return;
  sendFileAP("/logs.html");
}

static void handleLogsData()
{
  if (!registrado_ap)
  {
    serverAP.send(401, "application/json; charset=utf-8",
                  "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }
  lastActivityTime_ap = millis();

  uint32_t since = (uint32_t)(serverAP.hasArg("since") ? serverAP.arg("since").toInt() : 0);
  uint32_t next = 0;
  String json = logbuf_get_json_since(since, next);

  serverAP.send(200, "application/json; charset=utf-8", json);
}

// ========================= FS Upload (AP) =========================

static void handleFsPage()
{
  if (!requireAuthAP()) return;
  sendFileAP("/ficheros.html");
}

static bool isAllowedFsExt(const String &filenameLower)
{
  // Manténlo coherente con Ethernet: aquí lo hago amplio (puedes recortar)
  return filenameLower.endsWith(".html") ||
         filenameLower.endsWith(".css")  ||
         filenameLower.endsWith(".js")   ||
         filenameLower.endsWith(".png")  ||
         filenameLower.endsWith(".ico")  ||
         filenameLower.endsWith(".jpg")  ||
         filenameLower.endsWith(".jpeg") ||
         filenameLower.endsWith(".svg")  ||
         filenameLower.endsWith(".json");
}

static void handleFsUpload()
{
  if (!registrado_ap)
  {
    fsUploadOk_ap = false;
    fsUploadErr_ap = "No autorizado";
    return;
  }

  HTTPUpload &upload = serverAP.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    fsUploadOk_ap = false;
    fsUploadErr_ap = "";
    fsUploadTotal_ap = 0;
    fsUploadPath_ap = "";

    String filename = upload.filename;
    int slash = filename.lastIndexOf('/');
    int bslash = filename.lastIndexOf('\\');
    int cut = max(slash, bslash);
    if (cut >= 0) filename = filename.substring(cut + 1);
    filename.trim();

    if (filename.length() == 0 || filename.indexOf("..") >= 0)
    {
      fsUploadErr_ap = "Nombre de fichero inválido";
      return;
    }

    String lower = filename;
    lower.toLowerCase();
    if (!isAllowedFsExt(lower))
    {
      fsUploadErr_ap = "Tipo de archivo no permitido";
      return;
    }

    fsUploadPath_ap = "/" + filename;

    if (fsUploadFile_ap) fsUploadFile_ap.close();
    fsUploadFile_ap = LittleFS.open(fsUploadPath_ap, "w");
    if (!fsUploadFile_ap)
    {
      fsUploadErr_ap = "No se pudo crear el fichero en LittleFS";
      return;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadErr_ap.length() > 0) return;
    if (!fsUploadFile_ap)
    {
      fsUploadErr_ap = "Fichero no abierto";
      return;
    }

    size_t w = fsUploadFile_ap.write(upload.buf, upload.currentSize);
    if (w != upload.currentSize)
    {
      fsUploadErr_ap = "Error escribiendo en LittleFS";
      return;
    }
    fsUploadTotal_ap += upload.currentSize;
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile_ap) fsUploadFile_ap.close();

    if (fsUploadErr_ap.length() == 0 && fsUploadTotal_ap > 0)
      fsUploadOk_ap = true;
    else if (fsUploadErr_ap.length() == 0)
      fsUploadErr_ap = "Subida incompleta (0 bytes)";
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    fsUploadErr_ap = "Subida abortada por el cliente";
    if (fsUploadFile_ap) fsUploadFile_ap.close();
  }
}

static void handleFsUploadComplete()
{
  if (!registrado_ap)
  {
    redirectStatus("error", "Acceso denegado",
                   "Inicia sesión para actualizar el sistema de ficheros.",
                   "/", "Volver al login");
    return;
  }

  if (fsUploadErr_ap.length() > 0)
  {
    redirectStatus("error", "Subida fallida",
                   fsUploadErr_ap,
                   "/upload_fs", "Volver a ficheros");
    fsUploadOk_ap = false;
    fsUploadErr_ap = "";
    fsUploadPath_ap = "";
    fsUploadTotal_ap = 0;
    return;
  }

  if (!fsUploadOk_ap)
  {
    redirectStatus("error", "Subida fallida",
                   "No se pudo completar la subida del fichero.",
                   "/upload_fs", "Volver a ficheros");
    return;
  }

  redirectStatus("success", "Archivo actualizado",
                 "Archivo subido correctamente como " + fsUploadPath_ap + ". El dispositivo se reiniciará en unos instantes.",
                 "/menu", "Volver al menú");

  fsUploadOk_ap = false;
  fsUploadErr_ap = "";
  fsUploadPath_ap = "";
  fsUploadTotal_ap = 0;

  delay(150);
  ESP.restart();
}

// ========================= NotFound (estáticos) =========================

static void handleNotFound()
{
  String path = serverAP.uri();

  // Si existe, sirve el estático directamente (igual que Ethernet)
  if (path != "/" && LittleFS.exists(path))
  {
    sendFileAP(path);
    return;
  }

  redirectStatus("error", "404 - Página no encontrada",
                 "La ruta solicitada no existe. Revisa la URL o vuelve al menú.",
                 "/menu", "Volver al menú");
}

// ========================= Rutas =========================

void rutasServidor(WebServer &server_ref)
{
  server_ref.onNotFound(handleNotFound);

  server_ref.on("/", HTTP_GET, handleRoot);
  server_ref.on("/submit", HTTP_POST, handleSubmit);

  server_ref.on("/menu", HTTP_GET, handleMenu);

  server_ref.on("/reiniciar", HTTP_GET, handleReiniciarPage);
  server_ref.on("/reiniciar_dispositivo", HTTP_POST, handleReiniciarDispositivo);

  server_ref.on("/upload_firmware", HTTP_GET, handleFirmwarePage);
  server_ref.on("/upload_firmware", HTTP_POST, handleFirmwareUploadComplete, handleFirmwareUpload);

  server_ref.on("/upload_fs", HTTP_GET, handleFsPage);
  server_ref.on("/upload_fs", HTTP_POST, handleFsUploadComplete, handleFsUpload);

  server_ref.on("/logout", HTTP_GET, handleLogout);

  // Config (Preferences)
  server_ref.on("/config", HTTP_GET, handleConfigPage);
  server_ref.on("/config_save", HTTP_POST, handleConfigSave);

  // Status UI
  server_ref.on("/status", HTTP_GET, handleStatus);

  // Stats
  server_ref.on("/stats", HTTP_GET, handleStats);
  server_ref.on("/reset_aperturas", HTTP_POST, handleResetAperturas);

  // Logs
  server_ref.on("/logs", HTTP_GET, handleLogsPage);
  server_ref.on("/logs_data", HTTP_GET, handleLogsData);
}
