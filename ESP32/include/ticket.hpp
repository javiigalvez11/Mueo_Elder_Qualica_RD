#ifndef TICKET_HPP
#define TICKET_HPP

#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <Ethernet.h>
#include <map>

#include "time.hpp"
#include "http.hpp"
#include "definiciones.hpp"



#ifndef MARGEN_ANTICIPACION_S
#define MARGEN_ANTICIPACION_S 1200UL
#endif

struct TicketInfo {
  int      pasos;
  uint32_t segundosEntrada; // HH:MM -> segs del día
};

extern std::map<String, TicketInfo> ticketsLocales;
extern int  countTickets;
extern const int numMaxTickets;

bool cargarTicketsDesdeTextoPlano(const String& textoPlano);
bool descargarTicketsLocales();

int  verificarTicket(const String& codigoTicket); // 1 OK, 2 aún no, 3 no existe
void guardarTicketPendiente(const String& ticketCode, int pasosTotales);
bool hayTicketsPendientes();
bool reenviarTicketsPendientesComoBloque();

bool linkUp();
bool netOk();
#endif // TICKET_HPP