#ifndef DSSP3120_HPP
#define DSSP3120_HPP

#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "definiciones.hpp"

// --- Configuración Bus Único RS485 (HardwareSerial) ---
#define PIN_DSSP3120_TX   17
#define PIN_DSSP3120_RX   16
#define DSSP3120_BAUD     115200

namespace DSSP3120
{
    void begin();

    /**
     * @brief Lee del bus RS485, detecta el prefijo IN/OUT y limpia el código.
     * @param outRaw String donde se guarda el UID formateado o el código leído sin prefijo.
     * @param direccion Devuelve 1 si es Entrada (IN:), 2 si es Salida (OUT:).
     * @param kindOut Puntero al campo donde se guardará el tipo de QR detectado.
     * @return true si se ha completado una lectura válida.
     */
    bool readLine_parsed(String &outRaw, int &direccion, QRKind *kindOut);

    void flushInput();
}

#endif // DSSP3120_HPP