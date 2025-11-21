#include "gm65.hpp"
#include "rele.hpp"
#include "definiciones.hpp" // HEAD_ODOO, HEAD_TEC, debugSerie

static HardwareSerial *g_uart = nullptr;

// --------- Helpers privados ---------
static inline bool isAllDigits(const String &s)
{
  if (s.length() == 0)
    return false;
  for (size_t i = 0; i < s.length(); ++i)
  {
    if (!isDigit((unsigned char)s[i]))
      return false;
  }
  return true;
}

static inline bool isAllDigitsLen(const String &s, int n)
{
  return ((int)s.length() == n) && isAllDigits(s);
}

static inline bool isToken36_Odoo(const String &s)
{
  // 36 chars, hex y '-'
  if ((int)s.length() != 36)
    return false;
  for (size_t i = 0; i < s.length(); ++i)
  {
    const char c = s[i];
    const bool ok = (c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'f') ||
                    (c >= 'A' && c <= 'F') ||
                    (c == '-');
    if (!ok)
      return false;
  }
  return true;
}

static String getQueryParam(const String &url, const char *key)
{
  const int q = url.indexOf('?');
  const String query = (q >= 0) ? url.substring(q + 1) : url; // sin '?'
  const String ks = String(key) + "=";
  int pos = query.indexOf(ks);
  if (pos < 0)
    return "";
  pos += ks.length();
  const int amp = query.indexOf('&', pos);
  return (amp >= 0) ? query.substring(pos, amp) : query.substring(pos);
}

//---------------- Normalizador estricto ----------------
static QRKind parse_qr_normalizado(const String &raw, String &outCode)
{
  String s = raw;
  s.trim();

  // Quitar comillas envolventes "..." o '...'
  if (s.length() >= 2)
  {
    if ((s.startsWith("\"") && s.endsWith("\"")) ||
        (s.startsWith("'") && s.endsWith("'")))
    {
      s.remove(0, 1);
      s.remove(s.length() - 1);
    }
  }

  // 1) ODOO: https://tpv.museoelder.es/pos/ticket/validate?access_token=XXXXXXXX(36)
  if (s.startsWith(HEAD_ODOO))
  {
    const String token = getQueryParam(s, "access_token");
    if (isToken36_Odoo(token))
    {
      outCode = token; // solo el token
      return QR_ODOO;
    }
    return QR_UNKNOWN;
  }

  // 2) TEC: https://wptpv.museoelder.es/?event_qr_code=1&ticket_id=XXXXXX&event_id=YYYYYY
  if (s.startsWith(HEAD_TEC))
  {
    const String tid = getQueryParam(s, "ticket_id");
    const String eid = getQueryParam(s, "event_id");
    if (isAllDigitsLen(tid, 6) && isAllDigitsLen(eid, 6))
    {
      outCode = "ticket_id=" + tid + "&event_id=" + eid;
      return QR_TEC;
    }
    return QR_UNKNOWN;
  }

  // 3) Numérico puro de 19 dígitos
  if (isAllDigitsLen(s, 19))
  {
    outCode = s;
    return QR_MAGE;
  }

  return QR_UNKNOWN;
}

//---------------- API ----------------
void gm65_begin()
{
  Serial2.begin(GM65_BAUD, SERIAL_8N1, PIN_GM65_RX, PIN_GM65_TX);
  g_uart = &Serial2;
  if (debugSerie)
  {
    if (debugSerie)
    {
      Serial.println(F("[GM65] begin() OK"));
      Serial.printf("  UART2: RX=%d TX=%d @%lu\n", PIN_GM65_RX, PIN_GM65_TX, (unsigned long)GM65_BAUD);
    }
  }
}

// Lectura cruda (terminada en \r o \n), tolera hasta 512 chars
bool gm65_readLine(String &outRaw)
{
  static String buffer;

  if (!g_uart)
    return false;

  while (g_uart->available() > 0)
  {
    const char c = (char)g_uart->read();

    if (c == '\r' || c == '\n')
    {
      if (buffer.length() > 0)
      {
        outRaw = buffer;
        buffer = ""; // limpia buffer
        return true;
      }
      // Si \r\n sin contenido, ignorar y seguir
    }
    else
    {
      if ((uint8_t)c >= 0x20 || c == '\t')
      { // imprimibles
        if (buffer.length() < 512)
        {
          buffer += c;
        }
        else
        {
          // truncado suave: ignorar hasta fin de línea
        }
      }
    }
  }
  return false;
}

// Descarta entradas no válidas (devuelve false si no pasa normalización)
bool gm65_readLine_parsed(String &outCode, QRKind *kindOut)
{
  String raw;

  if (!gm65_readLine(raw))
    return false;

  if (raw == "ELDER_QUALICARD11")
    rele_open();

  const QRKind k = parse_qr_normalizado(raw, outCode);
  if (k == QR_UNKNOWN)
    return false;

  if (kindOut)
    *kindOut = k;
  return true;
}
