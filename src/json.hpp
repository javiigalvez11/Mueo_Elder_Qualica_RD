#pragma once
#include <Arduino.h>


// === Serializadores ===
void serializaInicio();
void serializaEstado();
void serializaQR();
void serializaPaso();
void serializaReportFailure();

// === Deserializadores ===
void descifraEstado();
void descifraQR();
void descifraPaso();
