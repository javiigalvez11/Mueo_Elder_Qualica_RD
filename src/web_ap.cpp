// web_ap.cpp
#include <LittleFS.h>
#include <Update.h>
#include "web_ap.hpp"
#include "definiciones.hpp"

// Prototypes for FS upload handlers (defined below)
void handleFsPage();
void handleFsUpload();
void handleFsUploadComplete();

// ======= ESTADO GLOBAL PARA OTA POR AP =======
static bool otaUploadOk_ap = false;
static String otaErrorMsg_ap = "";

// =============================================

void mensajeError()
{
  String mensaje = "<h1>404</h1>";
  mensaje += "Página No encontrada<br>";
  mensaje += "Intenta otra página<br>";
  serverAP.send(404, "text/html; charset=utf-8", mensaje);
}

void handleRoot()
{
  if (debugSerie)
    Serial.println("[AP] HTTP GET / -> handleRoot called");
  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    if (debugSerie)
      Serial.println("[AP] handleRoot: index.html NOT FOUND in LittleFS");
    serverAP.send(404, "text/plain", "Archivo index.html no encontrado en LittleFS!");
    return;
  }
  size_t fsize = file.size();
  if (debugSerie)
  {
    Serial.print("[AP] handleRoot: index.html size=");
    Serial.println((unsigned long)fsize);
  }

  String htmlContent = file.readString();
  file.close();

  // Sustituir marcador de error y enviar
  htmlContent.replace("pError", errorMessage_ap);
  if (debugSerie)
    Serial.println("[AP] handleRoot: sending index.html to client");
  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
  errorMessage_ap = "";
}

void handleSubmit()
{
  if (serverAP.hasArg("password"))
  {
    String enteredPassword = serverAP.arg("password");

    if (enteredPassword == credencialMaestra)
    {
      registrado_ap = true;
      lastActivityTime_ap = millis();
      errorMessage_ap = "";
      serverAP.sendHeader("Location", "/menu");
      serverAP.send(302, "text/plain", "");
    }
    else
    {
      errorMessage_ap = "<span style=\"color:red;\">Contraseña incorrecta, reintente</span>";
      serverAP.sendHeader("Location", "/");
      serverAP.send(302, "text/plain", "");
    }
  }
  else
  {
    errorMessage_ap = "<span style=\"color:red;\">Por favor, introduzca una contraseña.</span>";
    serverAP.sendHeader("Location", "/");
    serverAP.send(302, "text/plain", "");
  }
}

void handleMenu()
{
  if (registrado_ap)
  {
    lastActivityTime_ap = millis();
    File file = LittleFS.open("/menu.html", "r");
    if (!file)
    {
      serverAP.send(404, "text/plain", "Archivo menu.html no encontrado en LittleFS!");
      return;
    }
    String htmlContent = file.readString();
    file.close();
    serverAP.send(200, "text/html; charset=utf-8", htmlContent);
    lastActivityTime_ap = millis();
  }
  else
  {
    serverAP.sendHeader("Location", "/");
    serverAP.send(302, "text/plain", "");
  }
}

// Función para manejar la solicitud de la página de reiniciar
void handleReiniciarPage()
{
  if (!registrado_ap)
  {
    serverAP.sendHeader("Location", "/");
    serverAP.send(302);
    return;
  }
  File file = LittleFS.open("/reiniciar.html", "r");
  if (!file)
  {
    serverAP.send(500, "text/plain", "Error interno del servidor: reiniciar.html no encontrado.");
    return;
  }
  serverAP.streamFile(file, "text/html");
  file.close();
}

// Función para manejar la solicitud de reiniciar el dispositivo
void handleReiniciarDispositivo()
{
  if (!registrado_ap)
  {
    serverAP.send(401, "text/plain", "No autorizado.");
    return;
  }
  lastActivityTime_ap = millis();

  serverAP.send(200, "text/plain", "Reiniciando..."); // Envía respuesta antes de reiniciar
  delay(100);                                         // Pequeña pausa para asegurar que la respuesta se envía
  ESP.restart();                                      // Función para reiniciar el ESP32
}

void handleFirmwarePage()
{
  if (!registrado_ap)
  {
    serverAP.sendHeader("Location", "/");
    serverAP.send(302);
    return;
  }
  lastActivityTime_ap = millis();

  File file = LittleFS.open("/upload.html", "r");
  if (!file)
  {
    serverAP.send(500, "text/plain", "Error interno: upload.html no encontrado.");
    return;
  }

  String htmlContent = file.readString();
  file.close();

  // Reemplazas marcador VERSION_FIRMWARE con el contenido de enVersion
  htmlContent.replace("VERSION_FIRMWARE", enVersion);

  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
}

// ============ OTA FIRMWARE POR AP (tipo WebServer) ============
// Mejora: validación de nombre, control de errores y respuesta final
// ==============================================================
void handleFirmwareUpload()
{
  if (!registrado_ap)
  {
    // Error grave: usuario no autenticado
    serverAP.send(401, "text/plain", "No autorizado");
    return;
  }

  HTTPUpload &upload = serverAP.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    otaUploadOk_ap = false;
    otaErrorMsg_ap = "";

    String filename = upload.filename;
    // Normalizar nombre (quitar rutas)
    int slash = filename.lastIndexOf('/');
    int bslash = filename.lastIndexOf('\\');
    int cut = max(slash, bslash);
    if (cut >= 0)
      filename = filename.substring(cut + 1);
    filename.trim();

    if (debugSerie)
      Serial.printf("Iniciando actualización: %s\n", filename.c_str());

    // Nombre esperado (puedes usar nombrePlaca si quieres que sea dinámico)
    String expected = "Elder.bin"; // o String(nombrePlaca) + ".bin";
    expected.trim();

    if (filename != expected)
    {
      otaErrorMsg_ap = String("Nombre de fichero inválido. Debe ser: ") + expected;
      if (debugSerie)
        Serial.println(otaErrorMsg_ap);
      // No llamamos a Update.begin → simplemente ignoramos la OTA,
      // handleFirmwareUploadComplete se encargará de responder con error.
      return;
    }

    // Iniciar la OTA (tamaño desconocido, la partición OTA se encarga)
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      Update.printError(Serial);
      otaErrorMsg_ap = "Update.begin() falló al iniciar la OTA";
      return;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    // Solo escribimos si no hay error previo
    if (otaErrorMsg_ap.length() == 0)
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        Update.printError(Serial);
        otaErrorMsg_ap = "Error escribiendo firmware en flash";
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    // Finalizar OTA si no hubo errores previos
    if (otaErrorMsg_ap.length() == 0)
    {
      if (Update.end(true))
      {
        otaUploadOk_ap = true;
        if (debugSerie)
          Serial.printf("Actualización completada, bytes escritos: %u\n", upload.totalSize);
      }
      else
      {
        Update.printError(Serial);
        otaErrorMsg_ap = "Update.end() falló al finalizar la OTA";
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED)
  {
    otaErrorMsg_ap = "Carga OTA abortada por el cliente";
    Update.end();
  }
}

// Responde tras completar la carga OTA
void handleFirmwareUploadComplete()
{
  if (!registrado_ap)
  {
    serverAP.send(401, "text/plain", "No autorizado");
    return;
  }

  // Si ya tenemos mensaje de error específico, lo devolvemos
  if (otaErrorMsg_ap.length() > 0)
  {
    serverAP.send(500, "text/plain", String("Error en actualización: ") + otaErrorMsg_ap);
    // Limpiar estado para próximas OTAs
    otaUploadOk_ap = false;
    otaErrorMsg_ap = "";
    return;
  }

  // Chequear bandera y estado interno de Update
  if (!otaUploadOk_ap || Update.hasError())
  {
    serverAP.send(500, "text/plain", "Error en actualización");
    otaUploadOk_ap = false;
    otaErrorMsg_ap = "";
    return;
  }

  serverAP.send(200, "text/plain", "Actualización exitosa. Reiniciando...");
  otaUploadOk_ap = false;
  otaErrorMsg_ap = "";
  delay(100);
  ESP.restart();
}

// ********************************************************* LOG OUT ********************************************************************************
void handleLogout()
{
  registrado_ap = false;
  errorMessage_ap = "";
  serverAP.sendHeader("Location", "/");
  serverAP.send(302, "text/plain", "");
}

void rutasServidor(WebServer &server_ref)
{
  server_ref.onNotFound(mensajeError);
  server_ref.on("/", HTTP_GET, handleRoot);
  server_ref.on("/submit", HTTP_POST, handleSubmit);
  server_ref.on("/menu", HTTP_GET, handleMenu);
  server_ref.on("/reiniciar", HTTP_GET, handleReiniciarPage);
  server_ref.on("/reiniciar_dispositivo", HTTP_POST, handleReiniciarDispositivo);
  server_ref.on("/upload_firmware", HTTP_GET, handleFirmwarePage);
  // Importante: handler final + handler de subida
  server_ref.on("/upload_firmware", HTTP_POST, handleFirmwareUploadComplete, handleFirmwareUpload);
  server_ref.on("/upload_fs", HTTP_GET, handleFsPage);
  server_ref.on("/upload_fs", HTTP_POST, handleFsUploadComplete, handleFsUpload);
  server_ref.on("/logout", HTTP_GET, handleLogout);
}

// ========================= FS upload (AP) =========================
void handleFsPage()
{
  if (!registrado_ap)
  {
    serverAP.sendHeader("Location", "/");
    serverAP.send(302, "text/plain", "");
    return;
  }
  lastActivityTime_ap = millis();
  File file = LittleFS.open("/ficheros.html", "r");
  if (!file)
  {
    serverAP.send(500, "text/plain", "Error interno: ficheros.html no encontrado.");
    return;
  }
  String htmlContent = file.readString();
  file.close();
  serverAP.send(200, "text/html; charset=utf-8", htmlContent);
}

void handleFsUpload()
{
  if (!registrado_ap)
  {
    serverAP.send(401, "text/plain", "No autorizado");
    return;
  }
  HTTPUpload &upload = serverAP.upload();
  static File fsUploadFile;
  static size_t totalWritten = 0;

  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    // Normalizar nombre (quitar rutas)
    int slash = filename.lastIndexOf('/');
    int bslash = filename.lastIndexOf('\\');
    int cut = max(slash, bslash);
    if (cut >= 0)
      filename = filename.substring(cut + 1);
    filename.trim();
    if (debugSerie)
      Serial.printf("handleFsUpload: start file=%s size=%u\n", filename.c_str(), upload.totalSize);

    // Basic safety checks
    if (filename.length() == 0 || filename.indexOf("..") >= 0 || filename.startsWith("/"))
    {
      serverAP.send(400, "text/plain", "Nombre de fichero inválido");
      return;
    }
    // Validate extension
    String ext = "";
    int dot = filename.lastIndexOf('.');
    if (dot >= 0)
      ext = filename.substring(dot + 1);
    ext.toLowerCase();
    if (!(ext == "html" || ext == "json"))
    {
      serverAP.send(400, "text/plain", "Extensión no permitida. Solo .html y .json");
      return;
    }

    // Open (or create) file in LittleFS for writing (overwrite if exists)
    String path = "/" + filename;
    if (fsUploadFile)
    {
      fsUploadFile.close();
    }
    fsUploadFile = LittleFS.open(path, "w");
    if (!fsUploadFile)
    {
      serverAP.send(500, "text/plain", "No se pudo abrir/crear el fichero en LittleFS");
      return;
    }
    totalWritten = 0;
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (!fsUploadFile)
    {
      serverAP.send(500, "text/plain", "Fichero no abierto para escritura");
      return;
    }
    size_t w = fsUploadFile.write(upload.buf, upload.currentSize);
    totalWritten += upload.currentSize;
    if (debugSerie)
      Serial.printf("handleFsUpload: chunk write: %u bytes, totalWritten=%u\n", upload.currentSize, (unsigned)totalWritten);
    if (w != upload.currentSize)
    {
      if (debugSerie)
        Serial.printf("handleFsUpload: write error wrote=%u expected=%u\n", (unsigned)w, (unsigned)upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {
      fsUploadFile.close();
    }
    if (debugSerie)
      Serial.printf("handleFsUpload: upload end, totalWritten=%u\n", (unsigned)totalWritten);
    serverAP.send(200, "text/plain", String("Fichero subido: ") + upload.filename + " (" + String((unsigned)totalWritten) + " bytes)");
    // Reiniciar para aplicar cambios en LittleFS
    delay(100);
    ESP.restart();
  }
}

void handleFsUploadComplete()
{
  // No hace falta lógica extra: respondemos ya en handleFsUpload().
}
