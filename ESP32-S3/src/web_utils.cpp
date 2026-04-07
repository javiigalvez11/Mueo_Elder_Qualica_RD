// web_utils.cpp
#include <Arduino.h>

#include "web_utils.hpp"
#include "logBuf.hpp" // Si usas logs dentro de las validaciones
#include "definiciones.hpp"

String htmlEscape(String s)
{
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}


bool isDeviceIdOK(const String &s)
{
  if (s.length() != 5)
  {
    if (debugSerie)
    {
      Serial.println("Tamaño de deviceId: " + String(s.length()));
      Serial.println("isDeviceIdOK: longitud incorrecta");
    }

    logbuf_pushf(("Tamaño de deviceId: " + String(s.length())).c_str());
    logbuf_pushf("isDeviceIdOK: longitud incorrecta");
    return false;
  }
  if (!(s[0] == 'M' && s[1] == 'E'))
  {
    if (debugSerie)
    {
      Serial.println("isDeviceIdOK: prefijo incorrecto");
    }
    logbuf_pushf("isDeviceIdOK: prefijo incorrecto");
    return false;
  }
  for (int i = 2; i < 5; i++)
  {
    if (s[i] < '0' || s[i] > '9')
      return false;
  }
  if (debugSerie)
  {
    Serial.println("isDeviceIdOK: OK");
  }
  logbuf_pushf("isDeviceIdOK: OK");
  return true;
}

bool isIPv4OK(const String &ip)
{
  int parts = 0;
  int start = 0;

  for (int i = 0; i <= (int)ip.length(); i++)
  {
    if (i == (int)ip.length() || ip[i] == '.')
    {
      if (i == start)
        return false; // octeto vacío
      if (i - start > 3)
        return false; // más de 3 dígitos

      int val = 0;
      for (int j = start; j < i; j++)
      {
        char c = ip[j];
        if (c < '0' || c > '9')
          return false;
        val = val * 10 + (c - '0');
      }

      if (val < 0 || val > 255)
      {
        if (debugSerie)
          Serial.println("isIPv4OK: octeto fuera de rango: " + String(val));
        logbuf_pushf(("isIPv4OK: octeto fuera de rango: " + String(val)).c_str());
        return false;
      }

      parts++;
      start = i + 1;
    }
  }

  if (debugSerie)
    Serial.println("isIPv4OK: OK");
  return (parts == 4);
}

bool isUrlBaseOK(const String &u)
{
  if (u.length() == 0 || u.length() > 100)
    return false;
  if (!(u.startsWith("http://") || u.startsWith("https://")))
    return false;
  return true;
}



String getQueryParam(const String &pathWithQuery, const String &key)
{
  int q = pathWithQuery.indexOf('?');
  if (q < 0)
    return "";

  String query = pathWithQuery.substring(q + 1);
  String needle = key + "=";

  int p = query.indexOf(needle);
  if (p < 0)
    return "";

  int start = p + needle.length();
  int end = query.indexOf('&', start);
  if (end < 0)
    end = query.length();

  return urlDecode(query.substring(start, end));
}


// Encode para meter mensajes en querystring de /status
String urlEncode(const String &s)
{
  const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(s.length() * 2);

  for (int i = 0; i < (int)s.length(); i++)
  {
    uint8_t c = (uint8_t)s[i];

    // seguros
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~')
    {
      out += (char)c;
    }
    else if (c == ' ')
    {
      out += '+';
    }
    else
    {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
}


String urlDecode(const String &s)
{
  String out;
  out.reserve(s.length());

  for (int i = 0; i < (int)s.length(); i++)
  {
    char c = s[i];
    if (c == '+')
      out += ' ';
    else if (c == '%' && i + 2 < (int)s.length())
    {
      auto hex = [](char x) -> int
      {
        if (x >= '0' && x <= '9')
          return x - '0';
        if (x >= 'a' && x <= 'f')
          return x - 'a' + 10;
        if (x >= 'A' && x <= 'F')
          return x - 'A' + 10;
        return -1;
      };

      int a = hex(s[i + 1]), b = hex(s[i + 2]);
      if (a >= 0 && b >= 0)
      {
        out += char((a << 4) | b);
        i += 2;
      }
      else
        out += c;
    }
    else
      out += c;
  }
  return out;
}
