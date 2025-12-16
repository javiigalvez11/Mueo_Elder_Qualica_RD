// ============================================================
// main.cpp — ME001 (Entrada) — Integrado con W5500 + GM65 + FreeRTOS
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <WebServer.h>

#include "types.hpp"
#include "definiciones.hpp"
#include "time.hpp"   // syncHoraInicio(), loop_time_sync(), segundosActualesDelDia()
#include "W5500.hpp"  // begin_ethernet()
#include "http.hpp"   // getInicio(), getEstado(), postTicket(), postPaso()
#include "gm65.hpp"   // gm65_begin(), gm65_readLine_parsed(...)
#include "rele.hpp"   // rele_begin(), rele_open(), rele_close()
#include "json.hpp"   // serializa*/descifra* + g_validateOutcome
#include "ticket.hpp" // verificarTicket(), guardarTicketPendiente(), reenviarTicketsPendientesComoBloque()
#include "web_ap.hpp"
#include "web_eth.hpp"
#include "ficheros.hpp" // ensureUpdateFsPage()
#include "archivos.hpp" // cargaConfig()

// ------------------------------
// Servidor web global (coincidir con web.cpp)
// ------------------------------
WebServer serverAP(8080);

// ------------------------------
// Colas / Estado local de la tarea
// ------------------------------
static QueueHandle_t qToNet = nullptr; // IO -> NET
static String entradaPendiente = "";   // barcode en validación

// ------------------------------
// Prototipos
// ------------------------------
static void realizarPaso(const String &code);
static bool validateAndOpen(const String &code, uint32_t validateTimeoutMs = 5000);
static void taskNet(void *pv);
static void taskIO(void *pv);
static void handleSerialMenu();

static void startMaintenancePortal();
static void loopMaintenancePortal();

static void mountFS()
{
  if (debugSerie)
    Serial.println("[FS] Montando sistema de ficheros...");
  // Un único filesystem: LittleFS
  if (LittleFS.begin(false))
  {
    if (debugSerie)
      Serial.println("[FS] Sistema de ficheros montado correctamente");
    return;
  }

  if (debugSerie)
    Serial.println("[FS] LittleFS mount falló → intentando formatear");
  if (!LittleFS.begin(true))
  {
    Serial.println("[FS] ERROR: LittleFS formateo/monte falló");
    // aquí no return; el sistema seguirá y cargaConfig() debería detectar ausencia de ficheros
    return;
  }

  if (debugSerie)
    Serial.println("[FS] LittleFS formateado y montado");
}

// ============================================================
// setup / loop
// ============================================================
void setup()
{
  Serial.begin(115200);
  delay(300);

  // *** FS y config ***
  mountFS();

  // Cargar configuración (rellena DEVICE_ID, , credencialMaestra, serverURL, urlActualiza, red, etc.)
  cargaConfig();

  //Carga del html de actualización de ficheros
  ensureUpdateFsPage();  // Aquí se genera el fichero si no existe

  // *** Chequeo pin de mantenimiento ***
  pinMode(AP_PIN, INPUT_PULLUP);
  if (digitalRead(AP_PIN) == LOW)
  {
    // Arrancamos en modo mantenimiento: SOLO AP + WebServer del portal
    startMaintenancePortal();
    loopMaintenancePortal(); // Bloqueante hasta liberar el pin → reinicia
    // No debería volver
  }

  // === Flujo normal (sin portal) ===
  // Apaga WiFi si no lo usas en modo normal (evita conflictos y reduce consumo)
  WiFi.mode(WIFI_OFF);

  begin_ethernet(); // W5500 (usa config cargada)
  rele_begin();     // relé
  gm65_begin();     // lector QR GM65

  qToNet = xQueueCreate(10, sizeof(CmdMsg));
  xTaskCreatePinnedToCore(taskNet, "taskNet", 8192, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(taskIO, "taskIO", 8192, nullptr, 3, nullptr, 1);

  getInicio(); // primer /status → READY
  // syncHoraInicio(7000);

  delay(200);

  // unsigned long segs = segundosActualesDelDia();
  // Serial.printf("Segundos del día = %lu\n", segs);
}

void loop()
{
  // Nada: todo va en tareas
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================================
// Menú por puerto serie
// ============================================================
static void handleSerialMenu()
{
  if (!Serial.available())
    return;

  String data = Serial.readStringUntil('\n');
  data.trim();

  if (data == "serialOn")
  {
    debugSerie = 1;
  }
  else if (data == "serialOff")
  {
    debugSerie = 0;
  }
  else if (data == "actualiza")
  {
    actualizarFlag = 1;
  }
  else if (data == "reset")
  {
    restartFlag = 1;
  }
  else if (data == "abre")
  {
    rele_open();
  }
  else if (data == "cierra")
  {
    rele_close();
  }
  else if (data == "version")
  {
    Serial.println(enVersion);
  }
  else
  {
    Serial.println("Comando desconocido");
  }
}

// ============================================================
// Validación con backend + fallback OFFLINE
// ============================================================
static bool validateAndOpen(const String &code, uint32_t validateTimeoutMs)
{
  abortarPaso = 0;
  ultimoTicket = code;
  entradaPendiente = code;

  // ===== SIN INTERNET → OFFLINE
  if (!linkUp())
  {
    // loop_time_sync(15);
    int vr = verificarTicket(code); // 1=OK, 2=Aún no, 3=No existe
    if (vr == 1)
    {
      realizarPaso(code);
      guardarTicketPendiente(code, (pasosTotales > 0) ? pasosTotales : 1); // reenvío
      activaEntrada = 0;
      entradaPendiente.remove(0);
      if (debugSerie)
        Serial.println("[IO] OFFLINE → OK (apertura) y guardado en pendientes");
      activaConecta = 1;
      return true;
    }

    if (debugSerie)
    {
      if (vr == 2)
        Serial.println("[IO] OFFLINE: aún no es hora → rechazo");
      if (vr == 3)
        Serial.println("[IO] OFFLINE: ticket no existe → rechazo");
      return false;
    }
    pasoActual = -1;
    entradaPendiente.remove(0);
    activaEntrada = 0;
    return false;
  }

  // ===== CON INTERNET → ONLINE
  activaConecta = 0;

  g_validateOutcome = VNONE;
  g_lastEd = "";
  g_lastHttp = 0;

  CmdMsg v{};
  v.type = estadoMaquina;
  strlcpy(v.payload, code.c_str(), sizeof(v.payload));

  bool encolado = (xQueueSend(qToNet, &v, 0) == pdTRUE);
  if (!encolado && debugSerie)
  {
    Serial.println("[IO] validate: cola NET llena → no OFFLINE (hay Internet)");
  }

  bool autorizado = false;
  bool denegado = false;

  const uint32_t t0 = millis();
  const uint32_t maxWait = min<uint32_t>(validateTimeoutMs, (uint32_t)2000);

  if (encolado)
  {
    while (millis() - t0 < maxWait)
    {
      if (g_validateOutcome == VAUTH_IN || g_validateOutcome == VAUTH_OUT)
      {
        autorizado = true;
        break;
      }
      if (g_validateOutcome == VDENIED || g_validateOutcome == VERROR)
      {
        denegado = true;
        break;
      }
      if (activaEntrada)
      {
        autorizado = true;
        break;
      } // compat
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }

  if (autorizado)
  {
    if (debugSerie)
      Serial.println("[IO] ONLINE autorizado → abrir");
    realizarPaso(code);
    activaEntrada = 0;
    entradaPendiente.remove(0);
    return true;
  }

  // Denegado explícito: NO enviar PASS_TIMEOUT ni llamar validatePass
  if (denegado)
  {
    if (debugSerie)
    {
      Serial.print("[IO] ONLINE denegado → NO PASS_TIMEOUT. ed=");
      Serial.print(g_lastEd);
      Serial.print(" http=");
      Serial.println(g_lastHttp);
    }
    pasoActual = 0;
    activaEntrada = 0;
    entradaPendiente.remove(0);
    activaConecta = 1;
    return false;
  }

  // Timeout puro → registramos intento (PASS_TIMEOUT) si quieres telemetría
  if (debugSerie)
    Serial.println("[IO] ONLINE timeout → PASS_TIMEOUT");
  pasoActual = -1;
  ultimoPaso = code;
  CmdMsg to{};
  to.type = CMD_PASS_TIMEOUT;
  strlcpy(to.payload, code.c_str(), sizeof(to.payload));
  xQueueSend(qToNet, &to, 0);

  activaEntrada = 0;
  entradaPendiente.remove(0);
  activaConecta = 1;
  return false;
}

// ============================================================
// Portal de mantenimiento (AP + WebServer)
// ============================================================
static void startMaintenancePortal()
{

  if (debugSerie)
  {
    Serial.println("[AP] Iniciando portal de mantenimiento...");
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);

  // Fijar IP del AP si no está definida
  if (softIP == IPAddress(0, 0, 0, 0))
  {
    softIP = IPAddress(192, 168, 4, 1);
  }
  WiFi.softAPConfig(softIP, softIP, IPAddress(255, 255, 255, 0));

  bool ok = WiFi.softAP(ssid.c_str(), password.c_str());
  if (!ok)
  {
    if (debugSerie)
    {
      Serial.println("[AP] FALLO al iniciar SoftAP");
    }
  }
  else
  {
    if (debugSerie)
    {
      Serial.print("[AP] SSID: ");
      Serial.println(ssid);
      Serial.print("[AP] PASS: ");
      Serial.println(password);
      Serial.print("[AP] IP:   ");
      Serial.println(WiFi.softAPIP());
    }
  }

  // Rutas web (ya implementadas por ti en web.cpp)
  rutasServidor(serverAP);
  serverAP.begin();

  portalActivo = true;
}

static void loopMaintenancePortal()
{
  // Atiende el server mientras el pin 21 siga a GND
  while (portalActivo)
  {
    serverAP.handleClient();

    if (digitalRead(AP_PIN) == HIGH)
    {
      if (debugSerie)
        Serial.println("[AP] Pin liberado. Reiniciando para volver a modo normal...");
      delay(250);
      ESP.restart();
    }
    delay(2);
  }
}

// ============================================================
// Tarea NET (Core 0) —— Ethernet/HTTP + /status periódico + órdenes
// ============================================================
static void taskNet(void *pv)
{
  uint32_t lastStatus = millis();
  uint32_t lastTickets = millis();
  uint32_t lastPendSync = millis();

  for (;;)
  {
    // loop_time_sync(15);

    // 1) /status periódico
    if (activaConecta == 1 && (millis() - lastStatus) >= PERIOD_STATUS_MS)
    {
      lastStatus = millis();
      getEstado();
    }

    // 2) Reenvío de pendientes
    if ((millis() - lastPendSync) >= 5000)
    {
      lastPendSync = millis();
      if (netOk() && hayTicketsPendientes() && (!activaEntrada))
      {
        reenviarTicketsPendientesComoBloque();
      }
    }

    // 3) Descarga periódica de entradas
    if ((millis() - lastTickets) >= 60000 && netOk() && (!activaEntrada))
    {
      descargarTicketsLocales();
      // syncHoraInicio(3000);
      unsigned long segs = segundosActualesDelDia();
      if (debugSerie)
        Serial.printf("Segundos del día = %lu\n", segs);
      lastTickets = millis();
    }

    // 4) Mensajes IO -> NET
    CmdMsg msg{};
    if (xQueueReceive(qToNet, &msg, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      if (msg.type == CMD_VALIDATE_IN || msg.type == CMD_VALIDATE_OUT)
      {
        activaConecta = 0;
        ultimoTicket = String(msg.payload);
        postTicket(); // serializaQR() usará ultimoTicket
        activaConecta = 1;
      }
      else if (msg.type == CMD_PASS_OK)
      {
        ultimoPaso = msg.payload;
        if (linkUp())
        {
          postPaso(); // solo si hay Internet
        }
        else if (debugSerie)
        {
          if (debugSerie)
            Serial.println("[NET] PASS_OK offline → no se postea validatePass");
        }
        activaConecta = 1;
      }
      else if (msg.type == CMD_PASS_TIMEOUT)
      {
        estadoMaquina = CMD_PASS_TIMEOUT;
        pasoActual = -1;
        ultimoPaso = msg.payload;
        if (linkUp())
        {
          postPaso(); // registra intento fallido
        }
        else if (debugSerie)
        {
          if (debugSerie)
            Serial.println("[NET] PASS_TIMEOUT offline → no se postea validatePass");
        }
        activaConecta = 1;
      }
    }

    if (restartFlag == 1)
    {
      restartFlag = 0;
      ESP.restart();
    }

    /*if (actualizarFlag == 1) {
      actualiza();
      actualizarFlag = 0;
    }*/

    // Atender clientes HTTP
    webHandleClient();

    // Expirar sesión por inactividad
    if (registrado_eth && (millis() - lastActivityTime_eth > INACTIVITY_TIMEOUT_MS_ETH))
    {
      registrado_eth = false;
      errorMessage_eth = "";
      Serial.println("Sesión expirada por inactividad.");
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ============================================================
// Tarea IO (Core 1) —— GM65 + lógica de paso
// ============================================================
static void taskIO(void *pv)
{
  for (;;)
  {
    handleSerialMenu();

    // Si el servidor ya autorizó ENTRADA y hay código pendiente
    if (activaEntrada && entradaPendiente.length() > 0)
    {
      realizarPaso(entradaPendiente);
      entradaPendiente.remove(0);
      activaEntrada = 0;
    }
    else
    {
      // Leer GM65 SOLO si no hay paso en curso y la red está “conectable”
      String code;
      if (gm65_readLine_parsed(code) && activaConecta)
      {
        if (debugSerie)
        {
          Serial.print("[GM65][Barcode]: ");
          Serial.println(code);
        }

        if (sentido == "Entrada")
        {
          estadoMaquina = CMD_VALIDATE_IN;
          estadoPuerta = "201";
        }
        else
        {
          estadoMaquina = CMD_VALIDATE_OUT;
          estadoPuerta = "202";
        }
        if (!validateAndOpen(code, 3000))
        {
          if (debugSerie)
            Serial.println("[IO] Acceso DENEGADO");
          resetCycleReady();
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ============================================================
// Apertura/cierre físico (Relé) + confirmación
// ============================================================
static void realizarPaso(const String &code)
{
  const int pasosPrevios = contadorPasos;

  // Pulso de apertura (si tu driver no cierra solo, haz un close diferido)
  rele_open();
  // vTaskDelay(pdMS_TO_TICKS(300));
  // rele_close();

  // Asumimos paso OK inmediato (si tienes sensores, sustituye por lectura real)
  contadorPasos = contadorPasos + 1;

  if (abortarPaso)
  {
    if (debugSerie)
      Serial.println("[IO] Paso abortado.");
    rele_close();
    abortarPaso = 0;
    activaConecta = 1;
    pasoActual = 0;
  }
  else
  {
    const bool personaPaso = (contadorPasos > pasosPrevios);
    if (personaPaso)
    {
      pasoActual = contadorPasos;
      estadoMaquina = CMD_PASS_OK;

      CmdMsg ok{};
      ok.type = CMD_PASS_OK;
      strlcpy(ok.payload, code.c_str(), sizeof(ok.payload));
      xQueueSend(qToNet, &ok, 0);
    }
    else
    {
      pasoActual = -1;
      CmdMsg to{};
      to.type = CMD_PASS_TIMEOUT;
      strlcpy(to.payload, code.c_str(), sizeof(to.payload));
      xQueueSend(qToNet, &to, 0);
      contadorPasos = 0;
    }
  }
}
