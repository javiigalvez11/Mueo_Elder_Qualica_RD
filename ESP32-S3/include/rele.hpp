#ifndef RELE_HPP
#define RELE_HPP

#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "definiciones.hpp"

#define PIN_RELE_I 35
#define PIN_RELE_D 34

namespace rele
{
    void begin();
    void openEntry(); // abre el relé derecho
    void openExit();  // abre el relé izquierdo
    void close();      // cierra ambos relés
}
#endif // RELE_HPP