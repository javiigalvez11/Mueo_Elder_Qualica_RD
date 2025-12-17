#ifndef FICHEROS_HPP
#define FICHEROS_HPP

#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <pgmspace.h>

bool ensureWebPagesInLittleFS(bool formatOnFail);

#endif // FICHEROS_HPP