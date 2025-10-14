#include "json.hpp"
#include "config.hpp"
#include <ArduinoJson.h>



// =====================================================
// Serializadores
// =====================================================
void serializaInicio() {
  JsonDocument doc;
  doc["id"]       = DEVICE_ID;
  doc["comercio"] = COMERCIO;
  doc["status"]   = estadoPuerta;   // p.ej. "200"
  doc["version"]  = enVersion;

  outputInicio.remove(0);
  serializeJson(doc, outputInicio);
}

void serializaEstado() {
  JsonDocument doc;
  doc["id"]       = DEVICE_ID;
  doc["comercio"] = COMERCIO;
  doc["status"]   = estadoPuerta;
  doc["ec"]       = "READY";

  outputEstado.remove(0);
  serializeJson(doc, outputEstado);
}

void serializaQR() {
  JsonDocument doc;
  doc["id"]       = DEVICE_ID;
  doc["comercio"] = COMERCIO;
  doc["status"]   = estadoPuerta;
  doc["barcode"]  = ultimoTicket;
  doc["ec"]       = "CMD_VALIDATE";
  outputTicket.remove(0);
  serializeJson(doc, outputTicket);
}

void serializaPaso() {
  JsonDocument doc;
  doc["id"]      = DEVICE_ID;
  doc["status"]  = estadoPuerta;
  doc["barcode"] = ultimoPaso;
  doc["ec"]      = "CMD_PASS";
  doc["np"]      = String(pasoActual);
  doc["nt"]      = String(pasosTotales);

  outputPaso.remove(0);
  serializeJson(doc, outputPaso);

  ultimoPaso = "";
  if (pasoActual == -1) pasoActual = 0;
}

void serializaReportFailure() {
  JsonDocument doc;
  doc["id"]       = DEVICE_ID;
  doc["comercio"] = COMERCIO;
  doc["status"]   = "FAILURE";
  doc["ec"]       = errorCode;  // 500..503

#if !defined(JSON_MINIMO)
  if (errorCode == "500") {
    doc["time"] = String(segundosAhora);
  }
#endif

  outputInicio.remove(0); // reutilizamos buffer
  serializeJson(doc, outputInicio);
}

// =====================================================
// Deserializadores
// =====================================================
void descifraEstado() {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, estadoRecibido);
  if (err) return;

  String r  = doc["R"]  | "";
  String ec = doc["ec"] | "";

  if (r == "OK" && ec.length()) {
    cambiaPuerta = ec.toInt();
  }

  switch (cambiaPuerta) {
    case 201: activaEntrada = 1; break;
    case 202: activaSalida  = 1; break;
    case 203: abreContinua  = 1; break;
    case 290: actualizarFlag    = 1; break;
    case 300: abortarPaso   = 1; break;
    case 310: restartFlag       = 1; break;
    default: break;
  }
}

void descifraQR() {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, ticketRecibido);
  if (err) return;

  String r  = doc["R"]  | "";
  String ec = doc["ec"] | "";
  String nt = doc["nt"] | "";
  String np = doc["np"] | "";

  estadoTicket = ec;

  if (estadoTicket == "201") {
    activaConecta  = 0;
    ultimoTicket   = ultimoTicket; // ya lo pusiste antes de serializar
    pasosTotales   = nt.toInt();
    contadorPasos  = np.toInt();
    activaEntrada  = 1;
  } else if (estadoTicket == "202") {
    activaConecta  = 0;
    ultimoTicket   = ultimoTicket;
    pasosTotales   = nt.toInt();
    contadorPasos  = np.toInt();
    activaSalida   = 1;
  } else {
    estadoTicket = "null";
    ultimoTicket = "";
  }

  ticketRecibido = "";
}

void descifraPaso() {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, pasoRecibido);
  if (err) return;

  String r  = doc["R"]  | "";
  String ec = doc["ec"] | "";
  String nt = doc["nt"] | "";

  if (r == "OK") {
    cambiaPuerta  = ec.toInt();
    pasosTotales  = nt.toInt();
  }

  if (cambiaPuerta == 300) {
    abortarPaso = 1;
  }

  pasoRecibido = "";
}
