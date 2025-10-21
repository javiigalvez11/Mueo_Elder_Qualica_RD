// ============================================================
// main.cpp — ME002 (Entrada) — Integrado con Http + ApiClient + json
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <cstring>

#include "config.hpp"     
#include "Http.hpp"       // begin_wifi()
#include "http.hpp"       // getEstado(), postTicket(), postPaso(), reportFailure(), http_begin(), http_poll()
#include "GM65.hpp"       // gm65_begin(...), gm65_readLine(...)
#include "RS485.hpp"      // RS485::begin(...), ::openGateAlways(...), ::closeGate(...), ::leftOpen/rightOpen(...), ::poll(), ::resetDevice(...)
#include "Rele.hpp"       // rele_begin(), rele_open_entrada(), rele_open_salida()
#include "json.hpp"       // serializa*/descifra* y globals

// ------------------------------
// Colas
// ------------------------------
static QueueHandle_t qToNet = nullptr;   // IO -> NET
static QueueHandle_t qToIO  = nullptr;   // NET -> IO
static String entradaPendiente = "";     // barcode en validación

// ------------------------------
// Prototipos de funciones/tareas
// ------------------------------
static void handleSerialMenu();
static void realizarPaso(bool salida, const String& code);
static bool validateAndOpen(bool salida, const String& code, uint32_t validateTimeoutMs = 5000);
static void taskNet(void* pv);    // Core 0 (WiFi/HTTP)
static void taskIO(void* pv);     // Core 1 (GM65/RS485/Actuadores)


// ============================================================
// setup / loop
// ============================================================
void setup() {
  // --- Serial ---
  Serial.begin(115200);
  delay(300);

  // --- WiFi STA (IP fija) ---
  begin_wifi();   // usa http.cpp

  // --- RS-485 o Relé según modo ---
  if (aperturaMode == 1) { // 0=rele, 1=RS485
    RS485::begin(Serial1, RS485_BAUD, PIN_RS485_RX, PIN_RS485_TX);
  } else {
    rele_begin(); // alternativa por relé
  }

  // --- GM65 (UART2) ---
  gm65_begin(Serial2, PIN_GM65_RX, PIN_GM65_TX, GM65_BAUD);

  // --- Colas FreeRTOS ---
  qToNet = xQueueCreate(10, sizeof(CmdMsg));
  qToIO  = xQueueCreate(10, sizeof(CmdMsg));

  // --- WebServer (recibe POST de la esclava) ---
  http_begin(qToIO);

  // --- Tareas por núcleo ---
  xTaskCreatePinnedToCore(taskNet, "taskNet", 8192, nullptr, 2, nullptr, 0); // Core 0
  xTaskCreatePinnedToCore(taskIO,  "taskIO",  8192, nullptr, 3, nullptr, 1); // Core 1

  getInicio(); // primer /status al arrancar
}

void loop() {
  // No usamos loop(); todo va en tareas.
  vTaskDelay(pdMS_TO_TICKS(1000));
}


// ============================================================
// Menú por puerto serie
// ============================================================
static void handleSerialMenu() {
  if (!Serial.available()) return;

  String data = Serial.readStringUntil('\n');
  data.trim();

  if      (data == "serialOn")  { debugSerie = 1; }
  else if (data == "serialOff") { debugSerie = 0; }
  else if (data == "actualiza") { actualizarFlag = 1; }
  else if (data == "reset")     { restartFlag = 1; }
  else if (data == "abre")      { RS485::openGateAlways(MACHINE_ID); }
  else if (data == "cierra")    { RS485::closeGate(MACHINE_ID); }
  else if (data == "version")   { Serial.println(enVersion); }
  else                          { Serial.println("Comando Desconocido"); }
}


// ============================================================
// Helper: Validar con backend y abrir si autorizado
// salida=true -> espera status 202 (activaSalida); false -> 201 (activaEntrada)
// ============================================================
static bool validateAndOpen(bool salida, const String& code, uint32_t validateTimeoutMs) {
  // Pausar /status y preparar contexto
  activaConecta   = 0;
  abortarPaso     = 0;
  ultimoTicket    = code;           // lo usa serializaQR()
  entradaPendiente= code;           // por si quieres usarla luego
  pasoActual      = 0;              // reset marcador de paso

  // Pedir validación a NET
  CmdMsg v;
  v.type = CMD_VALIDATE;
  strlcpy(v.payload, code.c_str(), sizeof(v.payload));
  if (xQueueSend(qToNet, &v, 0) != pdTRUE) {
    if (debugSerie) Serial.println("[IO] validate: queue NET llena");
    activaConecta = 1;
    entradaPendiente.remove(0);
    return false;
  }

  // Esperar respuesta de descifraQR() (pone activaEntrada/activaSalida según 201/202)
  const uint32_t t0 = millis();
  bool autorizado = false;

  while (millis() - t0 < validateTimeoutMs) {
    if (!salida && activaEntrada) { autorizado = true; break; }
    if ( salida && activaSalida)  { autorizado = true; break; }
    RS485::poll();
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (!autorizado) {
    if (debugSerie) Serial.println("[IO] validate: NO autorizado o timeout");
    // Notificar timeout/no paso
    pasoActual = -1;
    ultimoPaso = code;
    CmdMsg to; to.type = CMD_PASS_TIMEOUT;
    strlcpy(to.payload, code.c_str(), sizeof(to.payload));
    xQueueSend(qToNet, &to, 0);

    // Limpieza
    activaEntrada = 0;
    activaSalida  = 0;
    entradaPendiente.remove(0);
    activaConecta = 1;
    return false;
  }

  // Autorizado -> abrir por la dirección solicitada
  if (debugSerie) {
    Serial.printf("[IO] validate: autorizado -> abrir %s\n", salida ? "SALIDA" : "ENTRADA");
  }
  realizarPaso(salida, code);   // tu función ya notifica PASS OK/TIMEOUT

  // Limpieza de flags locales
  activaEntrada = 0;
  activaSalida  = 0;
  entradaPendiente.remove(0);
  return true;
}


// ============================================================
// Tarea NET (Core 0) —— HTTP(S) + /status periódico + órdenes
// ============================================================
static void taskNet(void* pv) {
  uint32_t lastStatus = millis();

  for (;;) {
    // ------------------------------------------
    // A) Heartbeat /status cada PERIOD_STATUS_MS
    // ------------------------------------------
    if (activaConecta == 1) {
      if ((millis() - lastStatus) >= PERIOD_STATUS_MS) {
        lastStatus = millis();
        getEstado();   // serializaEstado() -> POST /status -> descifraEstado()
      }
    }

    // ------------------------------------------
    // B) Mensajes desde IO (VALIDATE / PASS / TIMEOUT)
    // ------------------------------------------
    CmdMsg msg;
    if (xQueueReceive(qToNet, &msg, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (msg.type == CMD_VALIDATE) {
        // Bloqueamos /status durante el ciclo de validación/paso
        activaConecta = 0;

        String code = String(msg.payload);
        ultimoTicket = code;

        // Valida ticket: serializaQR() + POST /validateQR + descifraQR()
        postTicket();

        // El resto del ciclo (abrir/esperar paso) lo hace IO tras ver las flags.
      }
      else if (msg.type == CMD_PASS_OK) {
        // Notificar paso OK
        ultimoPaso = msg.payload;
        postPaso();
      }
      else if (msg.type == CMD_PASS_TIMEOUT) {
        // Notificar que NO pasó nadie
        pasoActual = -1;
        ultimoPaso = msg.payload;
        postPaso();
      }
    }

    // Atender peticiones HTTP entrantes
    http_poll();

    // ------------------------------------------
    // C) Órdenes especiales (OTA / Reinicio)
    // ------------------------------------------
    if (restartFlag == 1) {
      if (debugSerie) { Serial.println("[ESP32] Reiniciando dispositivo…"); }
      RS485::resetDevice(MACHINE_ID);
      restartFlag = 0;
      ESP.restart();
    }

    if (actualizarFlag == 1) {
      if (debugSerie) { Serial.println("[ESP32] Buscando actualización OTA…"); }
      actualiza();
      actualizarFlag = 0;
    }

    // ------------------------------------------
    // D) Evitar busy loop
    // ------------------------------------------
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


// ============================================================
// Tarea IO (Core 1) —— GM65 + RS485 + lógica de paso
// ============================================================
static void taskIO(void* pv) {
  for (;;) {

    // A) Menú serie inmediato
    handleSerialMenu();

    // B) Procesar comandos NET->IO (p.ej. CMD_SLAVE_EXIT/ENTRY)
    {
      CmdMsg m;
      if (xQueueReceive(qToIO, &m, pdMS_TO_TICKS(10)) == pdTRUE) {
        const String code = String(m.payload);

        if (m.type == CMD_SLAVE_EXIT) {
          if (debugSerie) Serial.println("[IO] CMD_SLAVE_EXIT -> solicitar validación SALIDA");
          // Primero validar con el backend; si OK, abrir por SALIDA
          validateAndOpen(/*salida=*/true, code);
        }
        else if (m.type == CMD_SLAVE_ENTRY) {
          if (debugSerie) Serial.println("[IO] CMD_SLAVE_ENTRY -> solicitar validación ENTRADA");
          // Primero validar con el backend; si OK, abrir por ENTRADA
          validateAndOpen(/*salida=*/false, code);
        }
      }
    }

    // C) Si el servidor ya autorizó ENTRADA (descifraQR puso activaEntrada=1)
    //    y hay un código pendiente, ejecuta el paso de ENTRADA.
    // (Con validateAndOpen ya no es estrictamente necesario, pero lo dejamos por compat.)
    if (!activaSalida && activaEntrada && entradaPendiente.length() > 0) {
      realizarPaso(/*salida=*/false, entradaPendiente);
      entradaPendiente.remove(0);
      activaEntrada = 0;   // el ciclo de entrada concluye aquí
    }

    // D) Leer GM65 SOLO si no hay paso en curso
    if (!activaEntrada && !activaSalida) {
      String code;
      if (gm65_readLine(code)) {
        if (debugSerie) {
          Serial.print("[GM65] ");
          Serial.println(code);
        }

        // Pedimos validación a NET -> (descifraQR pondrá activaEntrada/salida si OK)
        CmdMsg m;
        m.type    = CMD_VALIDATE;
        strlcpy(m.payload, code.c_str(), sizeof(m.payload));
        xQueueSend(qToNet, &m, 0);

        // Guardamos el código para usarlo cuando el server autorice
        entradaPendiente = code;

        // Bloquea /status durante el ciclo
        activaConecta = 0;
      }
    }

    // E) Apertura continua (toggle) — solo RS485
    if (abreContinua) {
      if (estadoPuerta == "200") {
        estadoPuerta = "203";
        if (debugSerie) { Serial.println("Apertura continua: ON"); }
        RS485::openGateAlways(MACHINE_ID);
      } else {
        estadoPuerta = "200";
        if (debugSerie) { Serial.println("Apertura continua: OFF"); }
        RS485::closeGate(MACHINE_ID);
      }
      abreContinua = 0;
    }

    // F) Housekeeping RS485
    RS485::poll();

    // G) Yield
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}


// ============================================================
// Apertura/cierre físico (Relé o RS485)
// ============================================================
static void realizarPaso(bool salida, const String& code) {
  const int      pasosPrevios = contadorPasos;

  // Abrir
  if (aperturaMode == 0) { // 0=rele, 1=RS485
    if (!salida) {
      if (debugSerie) Serial.println("[IO] Apertura mediante RELE de ENTRADA");
      rele_open_entrada();
    } else {
      if (debugSerie) Serial.println("[IO] Apertura mediante RELE de SALIDA");
      rele_open_salida();
    }
  } else {
    if (salida)  RS485::leftOpen(MACHINE_ID, pasosTotales);
    else         RS485::rightOpen(MACHINE_ID, pasosTotales);

    const uint32_t t0 = millis();
    // Espera (paso / timeout / aborto)
    while (true) {
      RS485::poll();
      const bool personaPaso = (contadorPasos > pasosPrevios);
      const bool timeout     = ((millis() - t0) >= TIMEOUT_PASO_MS);
      const bool abortado    = (abortarPaso != 0);
      if (personaPaso || timeout || abortado) break;
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    RS485::closeGate(MACHINE_ID);
  }

  // Notificar a NET (usa los mismos endpoints que ya tienes)
  if (abortarPaso) {
    if (debugSerie) Serial.println("[IO] Paso abortado.");
    abortarPaso    = 0;
    // Rehabilitar /status
    activaConecta  = 1;
    pasoActual     = 0;
  } else {
    const bool personaPaso = (contadorPasos > pasosPrevios);

    if (personaPaso) {
      pasoActual = contadorPasos;
      CmdMsg ok; ok.type = CMD_PASS_OK;
      strlcpy(ok.payload, code.c_str(), sizeof(ok.payload));
      xQueueSend(qToNet, &ok, 0);
    } else {
      pasoActual = -1;
      CmdMsg to; to.type = CMD_PASS_TIMEOUT;
      strlcpy(to.payload, code.c_str(), sizeof(to.payload));
      xQueueSend(qToNet, &to, 0);
      contadorPasos = 0;
    }

    // Rehabilitar /status
    activaConecta = 1;
  }
}

