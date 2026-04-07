#include "DSSP3120.hpp"
#include "definiciones.hpp"
#include "config_params.hpp" // Para modoApertura, sentidoApertura, MACHINE_ID
#include "logBuf.hpp"
#include "RS485.hpp"
#include "rele.hpp"

static HardwareSerial *g_uart = &Serial1;

// ============================================================================
// 1. Helpers de parseo 
// ============================================================================

static inline bool isAllDigits(const String &s)
{
  if (s.length() == 0) return false;
  for (size_t i = 0; i < s.length(); ++i)
  {
    if (!isDigit((unsigned char)s[i])) return false;
  }
  return true;
}

static inline bool isValidTokenChar(char c) 
{
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F') ||
         (c == '-');
}

static bool isTokenOdoo_Flexible(String s) 
{
  s.trim();
  if (s.length() < 10) return false; 
  
  for (size_t i = 0; i < s.length(); ++i) {
    if (!isValidTokenChar(s[i])) return false;
  }
  return true;
}

static String getQueryParam(const String &url, const char *key)
{
  const int q = url.indexOf('?');
  const String query = (q >= 0) ? url.substring(q + 1) : url;
  const String ks = String(key) + "=";
  int pos = query.indexOf(ks);
  if (pos < 0) return "";
  
  pos += ks.length();
  const int amp = query.indexOf('&', pos);
  String res = (amp >= 0) ? query.substring(pos, amp) : query.substring(pos);
  res.trim();
  return res;
}

static QRKind parse_qr_normalizado(const String &raw, String &outCode)
{
  String s = raw;
  s.trim();

  if (s.length() >= 2) {
    if ((s.startsWith("\"") && s.endsWith("\"")) || (s.startsWith("'") && s.endsWith("'"))) {
      s.remove(0, 1);
      s.remove(s.length() - 1);
      s.trim();
    }
  }

  if (s.indexOf("tpv.museoelder.es") >= 0) {
    const String token = getQueryParam(s, "access_token");
    if (isTokenOdoo_Flexible(token)) {
      outCode = token;
      return QR_ODOO;
    }
  }

  if (s.indexOf("wptpv.museoelder.es") >= 0) {
    const String tid = getQueryParam(s, "ticket_id");
    const String eid = getQueryParam(s, "event_id");
    if (tid.length() > 0 && eid.length() > 0 && isAllDigits(tid) && isAllDigits(eid)) {
      outCode = "ticket_id=" + tid + "&event_id=" + eid;
      return QR_TEC;
    }
  }

  if (s.length() >= 17 && s.length() <= 20 && isAllDigits(s)) {
    outCode = s;
    return QR_MAGE;
  }

  return QR_UNKNOWN;
}

// ============================================================================
// 2. API DSSP3120 (Todo unificado en una sola función)
// ============================================================================
namespace DSSP3120 {

void begin()
{
  Serial1.begin(DSSP3120_BAUD, SERIAL_8N1, PIN_DSSP3120_RX, PIN_DSSP3120_TX);
  
  if (debugSerie) {
    Serial.println(F("[SETUP][DSSP3120] begin() OK"));
    Serial.printf("  UART1: RX=%d TX=%d @%lu\n", PIN_DSSP3120_RX, PIN_DSSP3120_TX, (unsigned long)DSSP3120_BAUD);
  }
}

// ÚNICA FUNCIÓN QUE SE LLAMA DESDE EL MAIN
bool readLine_parsed(String &outCode, int &direccion, QRKind *kindOut)
{
  static String buffer = "";
  if (!g_uart) return false;

  while (g_uart->available() > 0)
  {
    uint8_t byteIn = g_uart->read();

    // Ignoramos basura
    if (byteIn == 0x00 || byteIn == 0xFF) continue; 

    // Al detectar un salto de línea (Fin de lectura del código)
    if (byteIn == 0x0D || byteIn == 0x0A)
    {
      if (buffer.length() > 0)
      {
        String rawData = buffer;
        buffer = ""; // Reseteamos buffer para la siguiente tarjeta
        rawData.trim();

        if (rawData.length() == 0) continue;

        if (debugSerie) {
          Serial.print("[DSSP3120] Línea recibida: '");
          Serial.print(rawData);
          Serial.println("'");
        }
        logbuf_pushf("[DSSP3120] Línea recibida: '%s'", rawData.c_str());

        // =====================================================
        // PASO A: DETERMINAR LA DIRECCIÓN (IN / OUT)
        // =====================================================
        if (rawData.indexOf("IN:") != -1) {
          direccion = 1;
          rawData = rawData.substring(rawData.indexOf("IN:") + 3);
        } 
        else if (rawData.indexOf("OUT:") != -1) {
          direccion = 2;
          rawData = rawData.substring(rawData.indexOf("OUT:") + 4);
        } 
        else {
          direccion = 0; // Si no tiene IN ni OUT, lo descartamos
          if (debugSerie) Serial.println("[DSSP3120] Prefijo no detectado, ignorando.");
          logbuf_pushf("[DSSP3120] Prefijo no detectado, ignorando.");
          return false;
        }

        // =====================================================
        // PASO B: COMPROBAR LA LLAVE MAESTRA (BACKDOOR)
        // =====================================================
        if (rawData == "ELDER_QUALICARD11") 
        {
          logbuf_pushf("[DSSP3120] Llave maestra recibida. Abriendo torno directamente.");
          if (debugSerie) Serial.printf("[DSSP3120] Llave maestra. Dir: %d | Modo: %d\n", direccion, modoApertura);

          // Si usamos Relés
          if (modoApertura == 1) 
          {
            if (direccion == 1) { // Entrada
              rele::openEntry();
              entradasTotales++;
            } else {              // Salida
              rele::openExit();
              salidasTotales++;
            }
          }
          // Si usamos el bus RS485 del torno
          else 
          {
            if (direccion == 1) { // Entrada
              if (sentidoApertura == 0) RS485::leftOpen(MACHINE_ID, 1);
              else RS485::rightOpen(MACHINE_ID, 1);
            } else {              // Salida
              if (sentidoApertura == 0) RS485::rightOpen(MACHINE_ID, 1);
              else RS485::leftOpen(MACHINE_ID, 1);
            }
          }
          
          // Devolvemos FALSE para que el 'main.cpp' no lo mande a validar a la nube
          return false; 
        }

        // =====================================================
        // PASO C: NORMALIZAR TICKET Y ENVIAR AL MAIN
        // =====================================================
        const QRKind k = parse_qr_normalizado(rawData, outCode);
        
        if (k == QR_UNKNOWN) {
          if (debugSerie) Serial.println(F("[DSSP3120] QR Desconocido o No Válido"));
          logbuf_pushf("[DSSP3120] QR Desconocido: %s", rawData.c_str());
          return false;
        }

        if (kindOut) *kindOut = k;
        return true; // Devolvemos TRUE para que 'main.cpp' lo valide en el servidor
      }
    }
    // Llenamos el buffer con caracteres normales
    else if (byteIn >= 32 && byteIn <= 126)
    { 
      buffer += (char)byteIn;
      if (buffer.length() > 256) buffer = ""; // Protección anti-cuelgues
    }
  }
  return false;
}

void flushInput() {
  if (!g_uart) return;
  while (g_uart->available() > 0) g_uart->read();
}

} // namespace DSSP3120