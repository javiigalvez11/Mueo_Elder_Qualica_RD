#include "ticket.hpp"
#include "config.hpp"

// ====== Estado ======
std::map<String, TicketInfo> ticketsLocales;
int  countTickets = 0;
const int numMaxTickets = 500;

static bool g_httpOk = false;

// ====== helpers formatos ======
static bool is36AlphaDash(const String& s) {
  if ((int)s.length() != 36) return false;
  for (char c: s) {
    if (!((c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='-')) return false;
  }
  return true;
}
static bool isDigitsLen(const String& s, int n) {
  if ((int)s.length()!=n) return false;
  for (char c: s) if (!(c>='0'&&c<='9')) return false;
  return true;
}

// línea: "<CODE>:HH:MM;"
static bool parseLinea(const String& linea, String& codeOut, uint32_t& segsOut) {
  int p1 = linea.indexOf(':'); if (p1<=0) return false;
  int p2 = linea.indexOf(':', p1+1); if (p2<0) return false;
  int p3 = linea.indexOf(';', p2+1); if (p3<0) return false;

  String code = linea.substring(0,p1); code.trim();
  String sh   = linea.substring(p1+1,p2); sh.trim();
  String sm   = linea.substring(p2+1,p3); sm.trim();

  bool okCode = is36AlphaDash(code) || isDigitsLen(code,6) || isDigitsLen(code,19);
  if (!okCode) return false;

  int h = sh.toInt(), m = sm.toInt();
  if (h<0||h>23||m<0||m>59) return false;

  codeOut = code;
  segsOut = (uint32_t)h*3600UL + (uint32_t)m*60UL;
  return true;
}

// ---------------- Carga de tickets (texto plano local ó remoto) ----------------
bool cargarTicketsDesdeTextoPlano(const String& textoPlano) {
  ticketsLocales.clear();
  countTickets = 0;

  int start = 0;
  int end = textoPlano.indexOf('\n', start);
  if (end != -1) {
    String first = textoPlano.substring(start, end); first.trim();
    if (first == "OK") start = end + 1;
  }

  while (start < textoPlano.length() && countTickets < numMaxTickets) {
    end = textoPlano.indexOf('\n', start);
    if (end == -1) end = textoPlano.length();
    String linea = textoPlano.substring(start, end);
    linea.trim();
    start = end + 1;
    if (linea.length()==0) continue;

    String code; uint32_t segs=0;
    if (parseLinea(linea, code, segs)) {
      ticketsLocales[code] = { /*pasos*/1, segs };
      countTickets++;
    }
  }

  if (debugSerie) {
    Serial.printf("Se han cargado %d tickets con hora en RAM\n", countTickets);
    if (countTickets >= numMaxTickets) {
      Serial.printf("Se ha alcanzado el máximo permitido de entradas\n");
    }
  }
  return (countTickets > 0);
}

bool descargarTicketsLocales() {
  String texto;
  bool ok = getEntradas(texto); // http.cpp
  marcaHttpOk(ok);
  if (!ok) return false;

  bool cargado = cargarTicketsDesdeTextoPlano(texto);
  if (debugSerie) {
    if (cargado) Serial.println("[TICKETS] Descargados y cargados en RAM");
    else         Serial.println("[TICKETS] Respuesta sin entradas válidas");
  }
  return cargado;
}

// ---------------- Verificación local ----------------
int verificarTicket(const String& codigoTicket) {
  auto it = ticketsLocales.find(codigoTicket);
  if (it == ticketsLocales.end()) {
    if (debugSerie) Serial.printf("WARNING: Ticket no encontrado en memoria: %s\n", codigoTicket.c_str());
    return 3; // no existe
  }

  TicketInfo t = it->second;
  unsigned long ahora = segundosActualesDelDia();

  if (ahora + MARGEN_ANTICIPACION_S >= t.segundosEntrada) {
    ticketsLocales.erase(it); // consume para evitar reutilización
    if (debugSerie) {
      Serial.printf("Ticket válido (offline): %s | Entrada: %lu | Ahora: %lu | Pasos: %d\n",
        codigoTicket.c_str(), t.segundosEntrada, ahora, t.pasos);
    }
    // Si usas pasosTotales global, puedes asignarlo aquí:
    // pasosTotales = t.pasos;
    return 1;
  } else {
    unsigned long falta = (t.segundosEntrada > ahora) ? (t.segundosEntrada - ahora) : 0;
    if (debugSerie) {
      Serial.printf("🕒 Aún no es hora (offline): %s | Faltan %lu s\n",
        codigoTicket.c_str(), falta);
    }
    return 2; // aún no
  }
}

// ---------------- Pendientes ----------------
void guardarTicketPendiente(const String& ticketCode, int pasosTotales) {
  File file = SPIFFS.open(PENDIENTES_PATH, SPIFFS.exists(PENDIENTES_PATH) ? FILE_APPEND : FILE_WRITE);
  if (!file) {
    if (debugSerie) Serial.println("ERROR: No se pudo abrir pendientes.txt");
    return;
  }
  file.print(ticketCode);
  file.print(":");
  file.print(String(pasosTotales));
  file.print(";\n");
  file.close();

  if (debugSerie) {
    Serial.printf("Pendiente guardado → %s:%d\n", ticketCode.c_str(), pasosTotales);
  }
}


bool hayTicketsPendientes() {
  File file = SPIFFS.open(PENDIENTES_PATH, FILE_READ);
  if (!file) return false;
  while (file.available()) {
    String linea = file.readStringUntil('\n');
    linea.trim();
    if (linea.length()>0) { file.close(); return true; }
  }
  file.close();
  return false;
}

bool reenviarTicketsPendientesComoBloque() {
  File file = SPIFFS.open(PENDIENTES_PATH, FILE_READ);
  if (!file) return false;
  String contenido = file.readString();
  file.close();

  if (contenido.length() == 0) return true;

  bool ok = postPendientesBloque(contenido); // http.cpp
  if (ok) {
    SPIFFS.remove(PENDIENTES_PATH);
    if (debugSerie) Serial.println("[OFFLINE] Pendientes sincronizados y limpiados");
  } else if (debugSerie) {
    Serial.println("[OFFLINE] Fallo al sincronizar pendientes");
  }
  return ok;
}

// ---------------- Red ----------------
bool linkUp() {
  return Ethernet.linkStatus() == LinkON;
}

bool netOk() {
  return linkUp() && (Ethernet.localIP() != IPAddress(0,0,0,0)) && g_httpOk;
}
void marcaHttpOk(bool ok) { g_httpOk = ok; }
