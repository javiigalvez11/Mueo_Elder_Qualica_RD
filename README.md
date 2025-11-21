# Control de Accesos – Museo Elder (ESP32 + GM65)

Sistema de **control y gestión de accesos** para el **Museo Elder (Gran Canaria)** basado en **ESP32** y lectores **GM65** (QR/DataMatrix).  

Cada torno dispone de una placa de **entrada** y otra de **salida**, conectadas a la **intranet** del museo.  
Las placas **leen** el código, **validan** contra un **servidor interno** y, si procede, **habilitan el paso** (relé o RS-485) y notifican el **evento de paso**.

---

## ✨ Características principales

- ✅ Operación **desatendida**: lectura, validación y apertura automáticas  
- ⚡ **Baja latencia**: HTTP sobre red local  
- 🧾 **Trazabilidad**: registro de validaciones y pasos en el backend  
- 🔁 **Resiliencia**:
  - reintentos de comunicación  
  - envío en bloque de validaciones pendientes cuando vuelve la red  
- 📡 **Telemetría**: heartbeats periódicos con estado interno  
- 🔧 **Mantenimiento sencillo**:
  - Panel web por **Ethernet**
  - Portal **AP WiFi** local para configuración y actualización
- 🔌 Control del torno:
  - **Relé (contacto seco)**  
  - **RS-485** (si el torno lo soporta)

---

## 🧭 Arquitectura (alto nivel)

- **Placas físicas**
  - Hasta **10 placas ESP32-WROOM**, identificadas como `ME001` … `ME010`
  - Cada torno: 1 placa de **entrada** y 1 de **salida** (nomenclatura configurable)
- **Lectores**
  - Lectores **GM65** por UART (TTL 3.3V)
- **Red**
  - Conexión por **Ethernet (W5500)** a la red local `192.168.88.0/24`
  - Backend HTTP interno que expone la API de validación y registro

---

## 🌐 Direccionamiento de red

Las placas usan IPs fijas, **coherentes con su identificador**:

| Placa | IP              |
|------:|-----------------|
| ME001 | `192.168.88.1`  |
| ME002 | `192.168.88.2`  |
| ME003 | `192.168.88.3`  |
| ME004 | `192.168.88.4`  |
| ME005 | `192.168.88.5`  |
| ME006 | `192.168.88.6`  |
| ME007 | `192.168.88.7`  |
| ME008 | `192.168.88.8`  |
| ME009 | `192.168.88.9`  |
| ME010 | `192.168.88.10` |

La configuración (IP, máscara, gateway, DNS, URL del servidor, etc.) se almacena en **`config.json` dentro de LittleFS** y se puede modificar desde el panel de mantenimiento.

---

## 🔩 Hardware (pines por defecto)

> Configurables en `definiciones.hpp` según el montaje.

- **GM65 (UART2)**  
  - RX → `GPIO16`  
  - TX → `GPIO17`  

- **Salida a torno**
  - Relé (contacto seco) → `GPIO27`  
  - RS-485 (opcional):
    - TX → `GPIO4`  
    - RX → `GPIO5`  
    - DE/RE → `GPIO18`

- **Indicadores (opcionales)**
  - LED estado → `GPIO2`  
  - Buzzer → `GPIO15`  

> ⚠️ Si el TX del GM65 va a **5V**, hay que usar **divisor/level-shifter** hacia el RX del ESP32.

---

## 🗄️ Contrato de la API (backend ←→ placas)

Todas las llamadas HTTP del ESP32 van contra una **URL base**:

```text
serverURL = "http://<host_backend>:<puerto>/PTService/ESP32"
