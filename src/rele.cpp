#include "rele.hpp"
#include "config.hpp"

void rele_begin()
{
    pinMode(PIN_RELE_ENT, OUTPUT);
    // Ajusta según tu módulo de relé:
    // HIGH = reposo, LOW = activa (típico en opto con activo bajo)
    digitalWrite(PIN_RELE_ENT, HIGH);
    pinMode(PIN_RELE_SAL, OUTPUT);
    digitalWrite(PIN_RELE_SAL, LOW);
}

void rele_open(uint32_t ms)
{
    digitalWrite(PIN_RELE_SAL, HIGH);
    digitalWrite(PIN_RELE_ENT, LOW);
    delay(ms);
    digitalWrite(PIN_RELE_ENT, HIGH);
    digitalWrite(PIN_RELE_SAL, LOW);
}
