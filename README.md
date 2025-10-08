# Control de Accesos – Museo Elder (ESP32 + GM65)

Sistema de **control y gestión de accesos** para el **Museo Elder (Gran Canaria)** basado en **ESP32** y lectores **GM65** (QR/DataMatrix). Cada torno dispone de una placa de **entrada** y otra de **salida**, ambas conectadas a la **intranet (MikroTik)**. Las placas validan credenciales contra un **servidor interno** y, si procede, **habilitan el paso** (relé o RS-485) y notifican el **evento de paso**.

---

## ✨ Características
- ✅ Operación **desatendida**: lectura, validación y apertura automáticas  
- ⚡ **Baja latencia**: HTTP/HTTPS sobre red local  
- 🧾 **Trazabilidad**: registros de validaciones y pasos  
- 🔁 **Resiliencia**: reintentos y envío offline en bloque  
- 🔧 **Mantenible**: telemetría periódica y arquitectura modular  
- 🔌 **Apertura del torno**: **Relé (contacto seco)** o **RS-485** (si el torno lo soporta)

---

## 🧭 Arquitectura (alto nivel)
- **4 placas ESP32 WROOM**: T1(Entrada/Salida) + T2(Entrada/Salida)  
- **GM65** por UART (TTL 3.3V)  
- **API** en intranet para validar tickets y registrar eventos  
- **MikroTik hAP lite** como router/AP (LAN `192.168.88.0/24`)

---

## 🔩 Hardware (pines sugeridos)
- **GM65 (UART2)**: RX **GPIO16**, TX **GPIO17**  
- **Relé (contacto seco)**: **GPIO27**  
- **RS-485 (opcional)**: TX **GPIO4**, RX **GPIO5**, DE/RE **GPIO18**  
- **Opcionales**: LED **GPIO2**, buzzer **GPIO15**  
> Si el TX del GM65 es 5V, usar **divisor/level-shifter** hacia RX del ESP32.

---

## 🌐 Red (IPs recomendadas)
- **MikroTik**: `192.168.88.1`  
- **Placas**:  
  - `ME002` → `192.168.88.2` (T1 Entrada)  
  - `MS003` → `192.168.88.3` (T1 Salida)  
  - `ME004` → `192.168.88.4` (T2 Entrada)  
  - `MS005` → `192.168.88.5` (T2 Salida)

---

## 🗄️ API y JSON mínimos

**Campos comunes** en todas las peticiones desde las placas:  
`comercio` (string) · `id` (string) · `status` (string) · `ec` (string) · `barcode` (string, **solo cuando aplica**)

### Endpoints

**1) Heartbeat / estado** — `POST /api/status`
```json
{ "comercio":"MUSEO_ELDER", "id":"ME002", "status":"200", "ec":"OK" }
