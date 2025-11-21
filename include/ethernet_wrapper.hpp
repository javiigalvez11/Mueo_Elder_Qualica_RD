// ethernet_wrapper.hpp
#pragma once

#include <Ethernet.h>

// Wrapper para compatibilidad con la firma de Server en ESP32 core
class MyEthernetServer : public EthernetServer
{
public:
  MyEthernetServer(uint16_t port) : EthernetServer(port) {}
  void begin(uint16_t port = 0) override { EthernetServer::begin(); }
};
