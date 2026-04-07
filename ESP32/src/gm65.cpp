#include "definiciones.hpp" // HEAD_ODOO, HEAD_TEC, debugSerie
#include "gm65.hpp"
#include "rele.hpp"
#include "logBuf.hpp"

static HardwareSerial *g_uart = nullptr;

// --------- Helpers privados y relajados ---------

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

// Validación flexible: caracteres correctos y longitud mínima razonable
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
  res.trim(); // Limpieza preventiva
  return res;
}

//---------------- Normalizador Versión Robusta ----------------
static QRKind parse_qr_normalizado(const String &raw, String &outCode)
{
  String s = raw;
  s.trim();

  // Quitar comillas envolventes si existen
  if (s.length() >= 2)
  {
    if ((s.startsWith("\"") && s.endsWith("\"")) ||
        (s.startsWith("'") && s.endsWith("'")))
    {
      s.remove(0, 1);
      s.remove(s.length() - 1);
      s.trim();
    }
  }

  // 1) ODOO: Búsqueda por contenido de dominio (menos restrictivo que startsWith)
  if (s.indexOf("tpv.museoelder.es") >= 0)
  {
    const String token = getQueryParam(s, "access_token");
    if (isTokenOdoo_Flexible(token))
    {
      outCode = token;
      return QR_ODOO;
    }
  }

  // 2) TEC: Búsqueda por contenido de dominio
  if (s.indexOf("wptpv.museoelder.es") >= 0)
  {
    const String tid = getQueryParam(s, "ticket_id");
    const String eid = getQueryParam(s, "event_id");
    if (tid.length() > 0 && eid.length() > 0 && isAllDigits(tid) && isAllDigits(eid))
    {
      outCode = "ticket_id=" + tid + "&event_id=" + eid;
      return QR_TEC;
    }
  }

  // 3) Numérico puro (MAGE)
  if (s.length() >= 17 && s.length() <= 20 && isAllDigits(s))
  {
    outCode = s;
    return QR_MAGE;
  }

  return QR_UNKNOWN;
}

//---------------- API GM65 ----------------
namespace GM65
{
  void begin()
  {
    // Usamos Serial2 para el lector QR
    Serial2.begin(GM65_BAUD, SERIAL_8N1, PIN_GM65_RX, PIN_GM65_TX);
    g_uart = &Serial2;
    
    if (debugSerie)
    {
      Serial.println(F("[GM65] begin() OK"));
      Serial.printf("  UART2: RX=%d TX=%d @%lu\n", PIN_GM65_RX, PIN_GM65_TX, (unsigned long)GM65_BAUD);
    }
  }

  bool readLine(String &outRaw)
  {
    static String buffer;
    if (!g_uart) return false;

    while (g_uart->available() > 0)
    {
      const char c = (char)g_uart->read();

      if (c == '\r' || c == '\n')
      {
        if (buffer.length() > 0)
        {
          outRaw = buffer;
          outRaw.trim(); // Limpiar rastro de basura
          buffer = ""; 
          return true;
        }
      }
      else
      {
        // Solo aceptamos caracteres imprimibles (ASCII 32 a 126) y tabuladores
        if (((uint8_t)c >= 0x20 && (uint8_t)c <= 0x7E) || c == '\t')
        {
          if (buffer.length() < 512)
          {
            buffer += c;
          }
        }
      }
    }
    return false;
  }

  bool readLine_parsed(String &outCode, QRKind *kindOut)
  {
    String raw;
    if (!readLine(raw)) return false;

    if (debugSerie) {
      Serial.printf("[GM65] RAW: [%s] (Len: %d)\n", raw.c_str(), raw.length());
    }

    // Comando especial de apertura directa
    if (raw == "ELDER_QUALICARD11") {
      logbuf_pushf("[GM65] Comando de apertura recibido: ELDER_QUALICARD11");
      rele_open();
      return false;
      // No retornamos aquí para permitir que si es un QR procesable también lo haga
    }

    const QRKind k = parse_qr_normalizado(raw, outCode);
    
    if (k == QR_UNKNOWN) {
      if (debugSerie) Serial.println(F("[GM65] QR Desconocido o No Válido"));
      return false;
    }

    if (kindOut) *kindOut = k;
    return true;
  }
}