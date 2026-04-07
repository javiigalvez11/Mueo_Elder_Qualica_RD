// ===================================================================
// main.cpp — Museo Elder - Control de Torno - ESP32-S3 ETH Wavesahre
// ===================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <esp_now.h>

#include "definiciones.hpp"
#include "W5500.hpp"
#include "conexionWifi.hpp"
#include "http.hpp"
#include "DSSP3120.hpp"
#include "RS485.hpp"
#include "rele.hpp"
#include "json.hpp"
#include "web_eth.hpp"
#include "web_wifi.hpp"
#include "ficheros.hpp"
#include "config_prefs.hpp"
#include "config_params.hpp"
#include "logBuf.hpp"

// Servidor web global para WiFi
WebServer serverWiFi(8080);

// ------------------------------
// Colas / Estado local de la tarea
// ------------------------------
static QueueHandle_t qFromNet = nullptr; // NET -> IO (Respuesta validación)
static QueueHandle_t qToNet = nullptr;   // IO -> NET (Comandos y notificaciones)
static String entradaPendiente = "";

// ------------------------------
// Prototipos
// ------------------------------
static void taskNet(void *pv);
static void taskIO(void *pv);
static void handleSerialMenu();
static void abrirPuerta(int direccion);

static void mountFS()
{
    if (LittleFS.begin(false))
        return;
    if (!LittleFS.begin(true))
        Serial.println("[FS] ERROR: LittleFS failed");
}

// ============================================================
// setup / loop
// ============================================================
void setup()
{
    // Inicialización del sistema
    Serial.begin(115200);
    delay(1000); // Esperamos un momento a que el monitor serie se estabilice
    // Leemos la MAC base de los eFuses
    esp_read_mac(MAC, ESP_MAC_WIFI_STA);
    Serial.printf("MAC Obtenida: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

    // Inialización del sistema de archivos
    mountFS();
    // Inicialización del sistema de logs
    logbuf_begin();

    // ========================================================
    // 1) Carga de configuración de RED (TornoConfig)
    // ========================================================
    cfgBegin();
    cfgEnsureFirmwareDefaults();
    TornoConfig c = cfgLoad();
    cfgApplyToGlobals(c);

    // ========================================================
    // 2) Carga de parámetros TÉCNICOS RS485 (TornoParams)
    // ========================================================
    if (modoApertura == 0) // Solo cargamos parámetros si vamos a usar RS485, si no no tiene sentido cargar parámetros que no se van a usar
    {
        paramsBegin();                // Iniciamos el namespace "params" en Preferences
        paramsEnsureDefaults();       // Si la NVS está vacía o el firmware es nuevo, cargamos los 36 defaults
        TornoParams p = paramsLoad(); // Cargamos la estructura desde la flash
        paramsApplyToGlobals(p);      // Volcamos los valores a las variables independientes (p_machineId, p_openingMode, etc.)
    }

    logbuf_pushf("Sistema: Configuración y Parámetros cargados correctamente.");

    ensureWebPagesInLittleFS(true);

    // ========================================================
    // 3) Conexión a Red (WIFI o Ethernet)
    // ========================================================
    if (conexionRed == 0)
    {                  // MODO WIFI
        WIFI::begin(); // Inicia la conexión (asíncrono) y levanta el server web
    }
    else
    { // MODO ETHERNET
        W5500::begin();
        // En Ethernet esperamos un poco a que negocie físicamente
        if (Ethernet.localIP() != IPAddress(0, 0, 0, 0))
        {
            // Conexión exitosa inicial
        }
    }

    logbuf_pushf("[MAIN] Modo Pasillo: %d", modoPasillo);

    // En caso de que hagan falta placa maestra y esclava, inicializamos ESP-NOW en modo esclavo para recibir UIDs de la placa maestra del pasillo
    // if (modoPasillo != 1)
    // {
    //     if (esp_now_init() == ESP_OK)
    //         esp_now_register_recv_cb(OnDataRecv);
    // }

    logbuf_pushf("[MAIN] Modo Pasillo: %d", modoPasillo);

    // ========================================================
    // 4) Inicialización de periféricos (DSSP3120, RS485/Rele)
    // ========================================================
    DSSP3120::begin();

    // Inicializamos el sistema de apertura de puertas
    if (modoApertura == 0)
        RS485::begin();
    else
        rele::begin();

    qToNet = xQueueCreate(15, sizeof(CmdMsg));
    qFromNet = xQueueCreate(5, sizeof(ServerReply));

    xTaskCreatePinnedToCore(taskNet, "taskNet", 8192, nullptr, 3, nullptr, 0);
    xTaskCreatePinnedToCore(taskIO, "taskIO", 8192, nullptr, 5, nullptr, 1);
}

void loop()
{
    (pdMS_TO_TICKS(1000));
}

// ============================================================
// Tarea IO (Core 1)
// ============================================================
static void taskIO(void *pv)
{
    StateIO state = ST_IDLE;
    String codeRead = "";
    uint32_t waitStart = 0;
    int localPasosTotales = 0;
    int localPasosActuales = 0;
    int localDireccion = 0;
    uint32_t pasosRef = 0;      // El valor del contador justo al abrir
    uint32_t valorObjetivo = 0; // El valor que esperamos alcanzar (Ref + Totales)

    for (;;)
    {
        handleSerialMenu();
        // =============================================================================
        // 1) Coprobamos estado del torno (si es RS485) y notificamos fallos al backend
        // =============================================================================
        if (modoApertura == 0 && activaConecta == 1)
        {
            RS485::poll();
            RS485::StatusFrame st = RS485::getStatus();
            if (st.valid)
            {
                faultEvent = st.fault;
                gateStatus = st.gate; // El estado es 0x03 (normal), lo guardamos pero no es un fallo
                alarmEvent = st.alarm;
                powerSupplyVolt = st.vcc;
                leftCount = st.leftCount;
                rightCount = st.rightCount;

                // CORRECCIÓN: Quitamos (gateStatus != 0) de la condición de fallo
                bool hayProblema = (faultEvent != 0) || (alarmEvent != 0) || (powerSupplyVolt < 200);

                if (hayProblema && !errorNotificado)
                {
                    if (debugSerie)
                        Serial.printf("[MAIN][IO] ¡Fallo detectado! Fault:0x%02X | Alarm:0x%02X | VCC:%u\n", faultEvent, alarmEvent, powerSupplyVolt);

                    logbuf_pushf("[MAIN][IO] Fallo detectado! Alarma: 0x%02X", alarmEvent);

                    CmdMsg msgFallo;
                    msgFallo.type = CMD_FAIL_REPORT;

                    // Aseguramos que el mensaje va a la cola (aumentamos el timeout a 50ms por seguridad)
                    if (xQueueSend(qToNet, &msgFallo, pdMS_TO_TICKS(50)) == pdTRUE)
                    {
                        errorNotificado = true;
                    }
                    else
                    {
                        if (debugSerie)
                            Serial.println("[MAIN][IO] ERROR: Cola qToNet llena, no se pudo enviar FAIL_REPORT");
                    }
                }
                else if (!hayProblema && errorNotificado)
                {
                    errorNotificado = false;
                    if (debugSerie)
                        Serial.println("[MAIN][IO] Torno normalizado (Alarmas a 0).");
                    logbuf_pushf("[IO] Torno normalizado.");
                }
            }
        }

        // ====================================================================================
        // 3) Maquina de estados principal: Espera de lectura -> Validación -> Espera de paso
        // ====================================================================================
        switch (state)
        {
        case ST_IDLE:
            if (activaConecta == 0 && (estadoMaquina == CMD_VALIDATE_IN || estadoMaquina == CMD_VALIDATE_OUT))
            {
                localDireccion = (estadoMaquina == CMD_VALIDATE_IN) ? 1 : 2;
                waitStart = millis();
                state = ST_VALIDATING;
            }
            else if (activaConecta == 1 && iniciOk == true)
            {
                String codigoDetectado = "";
                int direccionDetectada = 0; // 1 = Entrada, 2 = Salida

                // Hacemos una única lectura. Si hay datos, la función rellena código y dirección.
                if (DSSP3120::readLine_parsed(codigoDetectado, direccionDetectada, &ultimoTipoQR))
                {
                    codeRead = codigoDetectado;
                    codeRead.trim();

                    // --- SI PASA EL FILTRO, ASIGNAR VALORES Y ENVIAR ---
                    localDireccion = direccionDetectada;
                    if (localDireccion == 1)
                    {
                        estadoMaquina = CMD_VALIDATE_IN;
                        estadoPuerta = 201;
                    }
                    else
                    {
                        estadoMaquina = CMD_VALIDATE_OUT;
                        estadoPuerta = 202;
                    }

                    activaConecta = 0;
                    CmdMsg msg;
                    msg.type = estadoMaquina;
                    strlcpy(msg.payload, codeRead.c_str(), sizeof(msg.payload));

                    xQueueReset(qFromNet);
                    xQueueSend(qToNet, &msg, pdMS_TO_TICKS(100));
                    waitStart = millis();
                    state = ST_VALIDATING;
                }
            }
            break;

        case ST_VALIDATING:
            ServerReply reply;
            if (xQueueReceive(qFromNet, &reply, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                if (reply.autorizado && reply.pasosTotales > 0)
                {
                    localPasosTotales = reply.pasosTotales;
                    pasosTotales = localPasosTotales;
                    localPasosActuales = 0;
                    pasosActuales = 0;

                    if (modoApertura == 0)
                    {
                        // IMPORTANTE: Captura de marca de agua
                        RS485::poll();
                        RS485::StatusFrame stStart = RS485::getStatus();

                        // --- Lógica de selección de contador de referencia ---
                        if (localDireccion == 1) // ENTRADA
                        {
                            // Si sentidoApertura es 0 => Left, si es 1 => Right
                            pasosRef = (sentidoApertura == 0) ? stStart.leftCount : stStart.rightCount;
                        }
                        else // SALIDA
                        {
                            // Si sentidoApertura es 0 => Right, si es 1 => Left
                            pasosRef = (sentidoApertura == 0) ? stStart.rightCount : stStart.leftCount;
                        }
                    }
                    else
                    {
                        if (localDireccion == 1)
                        {
                            pasosRef = entradasTotales; // En modo relé, no tenemos contador, así que asumimos que partimos de 0
                        }
                        else
                        {
                            pasosRef = salidasTotales;
                        }
                    }

                    valorObjetivo = pasosRef + localPasosTotales;

                    abrirPuerta(localDireccion);
                    waitStart = millis();
                    state = ST_WAITING_PASS;
                    if (debugSerie)
                        Serial.printf("[MAIN][IO] Apertura: Ref=%d, Obj=%d", pasosRef, valorObjetivo);

                    logbuf_pushf("[IO] Apertura: Ref=%d, Obj=%d", pasosRef, valorObjetivo);
                }
                else
                {
                    resetCycleReady();
                    state = ST_IDLE;
                }
            }
            else if (millis() - waitStart > SERVER_TIMEOUT)
            {
                resetCycleReady();
                state = ST_IDLE;
            }
            break;

        case ST_WAITING_PASS:

            uint32_t valorActualTorno = 0;
            bool datosValidos = false;

            if (modoApertura == 0)
            {
                // ========================================================
                // OBTENCIÓN DE DATOS - MODO RS485 (Lectura física)
                // ========================================================
                RS485::poll();
                RS485::StatusFrame st = RS485::getStatus();
                if (st.valid)
                {
                    datosValidos = true;
                    if (localDireccion == 1) // ENTRADA
                        valorActualTorno = (sentidoApertura == 0) ? st.leftCount : st.rightCount;
                    else // SALIDA
                        valorActualTorno = (sentidoApertura == 0) ? st.rightCount : st.leftCount;
                }
            }
            else
            {
                // ========================================================
                // OBTENCIÓN DE DATOS - MODO RELÉ (Contador Virtual)
                // ========================================================
                datosValidos = true;

                // Simulamos el paso físico de la persona:
                // Si han pasado 1.5s y aún no hemos alcanzado el objetivo, incrementamos el contador interno
                if ((millis() - waitStart > 1500) && ((pasosRef + localPasosActuales) < valorObjetivo))
                {
                    if (localDireccion == 1)
                        entradasTotales++;
                    else
                        salidasTotales++;
                }

                // Nuestro valor actual es la variable interna guardada
                if (localDireccion == 1)
                    valorActualTorno = entradasTotales;
                else
                    valorActualTorno = salidasTotales;
            }

            // ========================================================
            // EVALUACIÓN UNIFICADA (Idéntica para RS485 y Relé)
            // ========================================================
            if (datosValidos)
            {
                // Si el contador del torno (físico o virtual) ha avanzado
                if (valorActualTorno > (pasosRef + localPasosActuales))
                {
                    int incrementoReal = valorActualTorno - (pasosRef + localPasosActuales);

                    for (int i = 0; i < incrementoReal; i++)
                    {
                        if (localPasosActuales < localPasosTotales)
                        {
                            localPasosActuales++;
                            pasosActuales = localPasosActuales;
                            waitStart = millis(); // REINICIAMOS LOS 8 SEGUNDOS PARA LA SIGUIENTE PERSONA

                            // Si NO es el último paso, notificamos el paso intermedio
                            if (localPasosActuales < localPasosTotales)
                            {
                                CmdMsg msgStep;
                                msgStep.type = (localDireccion == 1) ? CMD_PASS_IN : CMD_PASS_OUT;
                                sprintf(msgStep.payload, "%d/%d", localPasosActuales, localPasosTotales);
                                xQueueSend(qToNet, &msgStep, pdMS_TO_TICKS(10));
                                logbuf_pushf("[IO] Paso Intermedio: %s", msgStep.payload);

                                // En modo relé, necesitamos dar un nuevo pulso para la siguiente persona
                                if (modoApertura == 1)
                                {
                                    if (localDireccion == 1)
                                        rele::openEntry();
                                    else
                                        rele::openExit();
                                }
                            }
                        }
                    }
                }

                // CONDICIÓN DE ÉXITO FINAL: El contador llegó al objetivo
                if (valorActualTorno >= valorObjetivo || localPasosActuales >= localPasosTotales)
                {
                    logbuf_pushf("[IO] Meta alcanzada (%d). Enviando CMD_PASS_OK.", valorActualTorno);
                    CmdMsg msgOk;
                    msgOk.type = CMD_PASS_OK;
                    xQueueSend(qToNet, &msgOk, pdMS_TO_TICKS(10));

                    if (modoApertura == 0)
                        RS485::closeGate(MACHINE_ID);
                    else
                        rele::close(); // Por seguridad, nos aseguramos de que el relé esté apagado

                    state = ST_IDLE;
                }
            }

            // ========================================================
            // CONDICIÓN DE TIMEOUT: 8 segundos de inactividad
            // ========================================================
            if (state == ST_WAITING_PASS && (millis() - waitStart > PASO_TIMEOUT))
            {
                logbuf_pushf("[IO] Timeout 8s. Pasaron %d de %d.", localPasosActuales, localPasosTotales);

                if (modoApertura == 0)
                    RS485::closeGate(MACHINE_ID);
                else
                    rele::close();

                CmdMsg msgTo;
                msgTo.type = CMD_PASS_TIMEOUT;
                xQueueSend(qToNet, &msgTo, pdMS_TO_TICKS(10));
                state = ST_IDLE;
            }
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================
// Tarea NET (Core 0)
// ============================================================
static void taskNet(void *pv)
{
    uint32_t lastStatus = millis();
    uint32_t lastInicioAttempt = 0;
    uint32_t lastDhcpMaintain = millis();
    uint32_t lastHealthCheck = millis();
    uint32_t lastWifiReconnect = millis(); // Para no saturar los reintentos WiFi
    uint32_t lastEthRetry = 0;

    uint8_t fallosConsecutivos = 0;
    bool prevLinkState = true;

    for (;;)
    {
        // 1. SERVIDOR WEB (Prioridad máxima para el portal)
        if (conexionRed == 0)
        {
            serverWiFi.handleClient();
        }
        else
        {
            webHandleClient();
            if (portalApActivo)
                serverWiFi.handleClient();
        }

        // 2. VIGILANTE DE RED FÍSICA / ENLACE
        bool currentLink = linkUp();

        // Si recuperamos cable o señal WiFi, reseteamos el estado de saludo al backend
        if (currentLink && !prevLinkState)
        {
            if (debugSerie)
                Serial.println("[NET] Enlace recuperado. Reintentando saludo...");
            // iniciOk = false;
        }
        prevLinkState = currentLink;

        // --- LÓGICA DE RECUPERACIÓN ETHERNET ---
        if (conexionRed == 1) { // Si estamos en modo Ethernet
            IPAddress ip = Ethernet.localIP();
            // Si hay cable pero la IP es 0.0.0.0, hay que reintentar
            if (currentLink && (ip == IPAddress(0,0,0,0))) {
                if (millis() - lastEthRetry > 10000) { // Reintentar cada 10 seg
                    lastEthRetry = millis();
                    W5500::reintentarDHCP();
                }
            }
        }

        // --- RECONEXIÓN WIFI STA (Solo si NO estamos ya en modo rescate) ---
        if (conexionRed == 0 && !currentLink && !portalApActivo)
        {
            if (millis() - lastWifiReconnect > 15000)
            {
                lastWifiReconnect = millis();
                if (debugSerie)
                    Serial.println("[NET] Buscando red WiFi configurada...");
                WIFI::reconnect();
            }
        }

        // 3. INTENTO DE SALUDO AL BACKEND (Solo si hay link y no estamos en AP)
        if (currentLink && !iniciOk && !portalApActivo)
        {
            if (millis() - lastInicioAttempt > 5000)
            {
                lastInicioAttempt = millis();
                getInicio();
            }
        }

        // 4. VIGILANTE DE CAÍDAS (LÓGICA DE LOS 5 INTENTOS)
        if (millis() - lastHealthCheck > 5000)
        {
            lastHealthCheck = millis();

            // Un sistema está sano si tiene cable/señal Y el backend ha respondido (iniciOk)
            bool isHealthy = currentLink && iniciOk;

            if (!isHealthy && !portalApActivo)
            {
                fallosConsecutivos++;
                if (debugSerie)
                    Serial.printf("[SISTEMA] Intento de conexión fallido (%d/10)\n", fallosConsecutivos);
            }
            else if (isHealthy)
            {
                fallosConsecutivos = 0;
            }

            // DISPARADOR DEL PORTAL AP (Tras 10 fallos consecutivos)
            if (fallosConsecutivos >= 10 && !portalApActivo)
            {
                logbuf_pushf("[SISTEMA] Red no disponible tras 10 intentos. Activando AP...");
                if (debugSerie)
                    Serial.println("[SISTEMA] 10 fallos alcanzados. Levantando portal de rescate...");

                // Limpiamos intentos previos de conexión para estabilizar el AP
                if (conexionRed == 0)
                    WiFi.disconnect();

                WIFI::startFallbackAP();

                // Si el sistema es de Ethernet, el servidor WiFi de configuración no estaba activo
                if (conexionRed == 1)
                {
                    setupWebWiFi();
                    serverWiFi.begin();
                }
                portalApActivo = true;
            }
        }

        // ======================================================
        // 5. LÓGICA PRINCIPAL DEL TORNO (Solo si hay conexión OK)
        // ======================================================
        if (currentLink && iniciOk)
        {
            CmdMsg msg{};
            if (xQueueReceive(qToNet, &msg, 0) == pdTRUE)
            {
                if (msg.type == CMD_FAIL_REPORT)
                {
                    reportFailure();
                }
                else if (msg.type == CMD_VALIDATE_IN || msg.type == CMD_VALIDATE_OUT)
                {
                    activaConecta = 0;
                    ultimoTicket = String(msg.payload);
                    postTicket();

                    ServerReply reply;
                    reply.autorizado = (g_validateOutcome == VAUTH_IN || g_validateOutcome == VAUTH_OUT);
                    reply.pasosTotales = (reply.autorizado) ? pasosTotales : 0;
                    xQueueSend(qFromNet, &reply, pdMS_TO_TICKS(50));

                    if (!reply.autorizado)
                        activaConecta = 1;
                }
                else if (msg.type == CMD_PASS_IN || msg.type == CMD_PASS_OUT)
                {
                    estadoMaquina = msg.type;
                    ultimoPaso = msg.payload;
                    postPaso();
                }
                else if (msg.type == CMD_PASS_OK)
                {
                    estadoPuerta = 205;
                    estadoMaquina = CMD_PASS_OK;
                    ultimoPaso = "OK";
                    postPaso();
                    DSSP3120::flushInput();
                    activaConecta = 1;
                }
                else if (msg.type == CMD_PASS_TIMEOUT)
                {
                    estadoPuerta = 206;
                    estadoMaquina = CMD_PASS_TIMEOUT;
                    ultimoPaso = "TIMEOUT";
                    postPaso();
                    DSSP3120::flushInput();
                    activaConecta = 1;
                }
            }

            // Latido (Status periódico)
            if (activaConecta == 1 && (millis() - lastStatus) >= PERIOD_STATUS_MS)
            {
                lastStatus = millis();
                getEstado();
            }

            if (restartFlag == 1)
            {
                restartFlag = 0;
                ESP.restart();
            }
        }

        // ======================================================
        // 6. CONTROL DE SESIÓN WEB
        // ======================================================
        if (registrado_eth && (millis() - lastActivityTime_eth > INACTIVITY_TIMEOUT_MS_ETH))
        {
            registrado_eth = false;
            logbuf_enable(false);
            logbuf_clear();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void abrirPuerta(int direccion)
{
    if (debugSerie)
        Serial.printf("[MAIN][IO] Abriendo puerta. Dirección: %d | ModoApertura: %d\n", direccion, modoApertura);

    if (modoApertura == 1) // --- MODO RELÉ ---
    {
        if (direccion == 1)
            rele::openEntry();
        else
            rele::openExit();
    }
    else // --- MODO RS485 ---
    {
        if (direccion == 1)
        {
            if (sentidoApertura == 0)
                RS485::leftOpen(MACHINE_ID, pasosTotales);
            else
                RS485::rightOpen(MACHINE_ID, pasosTotales);
        }
        else
        {
            if (sentidoApertura == 0)
                RS485::rightOpen(MACHINE_ID, pasosTotales);
            else
                RS485::leftOpen(MACHINE_ID, pasosTotales);
        }
    }
}

static void handleSerialMenu()
{
    if (!Serial.available())
        return;
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data == "reset")
        restartFlag = 1;
    else if (data == "abre")
        RS485::rightOpen(MACHINE_ID, 1);
    else if (data == "cierra")
        RS485::closeGate(MACHINE_ID);
}