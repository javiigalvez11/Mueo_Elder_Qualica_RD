#pragma once

// Inicializa (si hace falta) y atiende a los clientes HTTP
void webHandleClient();

// Helpers de rutas
void handleRoot(class EthernetClient &client);
void handleSubmit(class EthernetClient &client, const String &body);
void handleMenu(class EthernetClient &client);
void handleReiniciarPage(class EthernetClient &client);
void handleReiniciarDispositivo(class EthernetClient &client);
void handleFirmwarePage(class EthernetClient &client);
void handleLogout(class EthernetClient &client);

// NUEVO: páginas y upload de LittleFS
void handleFsPage(class EthernetClient &client);
void handleFsUpload(class EthernetClient &client, int contentLength, const String &contentType);
