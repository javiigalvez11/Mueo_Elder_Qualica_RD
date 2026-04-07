// web_utils.hpp
#pragma once
#include <Arduino.h>

String htmlEscape(String s);
bool isDeviceIdOK(const String &s);
bool isIPv4OK(const String &ip);
bool isUrlBaseOK(const String &u);

// También puedes mover aquí las funciones de URL encode/decode si quieres compartirlas
String urlDecode(const String &s);
String urlEncode(const String &s);
String getQueryParam(const String &pathWithQuery, const String &key); // Adaptada para ser genérica