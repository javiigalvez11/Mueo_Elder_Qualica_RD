#include "ficheros.hpp"


// ============================================================================
// 1) HTMLs en PROGMEM (UNO por fichero)
//    NOTA: Mantén los placeholders {{...}} tal cual (los remplazas en runtime).
// ============================================================================

static const char HTML_CONFIG[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">

<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Configuración del torno</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root {
      --dark: #292734;
      --dark-2: #1f1e28;
      --red: #eb3a3a;
      --red-2: #ff3a2d;
      --bg: #fafafa;
      --card: #ffffff;
      --line: #e6e6ea;
      --muted: #6b6a75;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--dark);
      font-family: "Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
    }

    /* ================= CABECERA GENERAL ================= */
    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark-2) 33%, var(--red) 66%, var(--red-2) 100%);
      padding: 22px 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      border-bottom: 1px solid var(--line);
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    /* Contenedor centrado */
    .page {
      min-height: calc(100vh - 96px);
      display: flex;
      align-items: flex-start;
      justify-content: center;
      padding: 28px 14px;
    }

    .card {
      width: 100%;
      max-width: 880px;
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, .08);
      overflow: hidden;
    }

    .card-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark-2) 33%, var(--red) 66%, var(--red-2) 100%);
      padding: 18px 18px;
      color: #fff;
    }

    .card-header h1 {
      margin: 0;
      font-size: 20px;
      font-weight: 700;
      letter-spacing: .2px;
    }

    .card-body {
      padding: 18px;
    }

    .msg {
      margin: 0 0 14px 0;
      padding: 12px 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
      color: var(--dark);
      font-size: 14px;
    }

    form {
      margin: 0;
    }

    label {
      display: block;
      margin: 12px 0 6px 0;
      font-weight: 700;
      font-size: 14px;
      color: var(--dark);
    }

    input,
    select {
      width: 100%;
      padding: 12px 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
      color: var(--dark);
      outline: none;
      font-size: 14px;
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    input:focus,
    select:focus {
      border-color: rgba(235, 58, 58, .8);
      box-shadow: 0 0 0 4px rgba(235, 58, 58, .12);
    }

    .grid-2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 14px;
    }

    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 16px;
    }

    button {
      border: none;
      border-radius: 10px;
      padding: 12px 16px;
      font-weight: 800;
      letter-spacing: .3px;
      cursor: pointer;
      color: #fff;
      background: linear-gradient(to right, var(--red-2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: background-position .25s ease, transform .06s ease;
    }

    button:hover {
      background-position: left bottom;
    }

    button:active {
      transform: translateY(1px);
    }

    a.btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      border-radius: 10px;
      padding: 12px 16px;
      font-weight: 800;
      letter-spacing: .3px;
      text-decoration: none;
      border: 1px solid var(--line);
      color: var(--dark);
      background: #fff;
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    a.btn:hover {
      border-color: rgba(41, 39, 52, .35);
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
    }

    .note {
      margin-top: 14px;
      padding-top: 12px;
      border-top: 1px solid var(--line);
      color: var(--muted);
      font-size: 13px;
      line-height: 1.35;
    }

    /* ================= CONTADOR (nuevo) ================= */
    .counter {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 14px 14px;
      border-radius: 12px;
      border: 1px solid var(--line);
      background: #fff;
      margin: 0 0 14px 0;
    }

    .counter-left {
      display: flex;
      flex-direction: column;
      gap: 4px;
    }

    .counter-title {
      font-size: 13px;
      color: var(--muted);
      font-weight: 800;
    }

    .counter-value {
      font-size: 26px;
      font-weight: 900;
      letter-spacing: .4px;
      color: var(--dark);
      line-height: 1.1;
    }

    .counter-sub {
      font-size: 12px;
      color: var(--muted);
    }

    .counter-btn {
      border: none;
      border-radius: 10px;
      padding: 12px 14px;
      font-weight: 800;
      cursor: pointer;
      color: #fff;
      background: linear-gradient(to right, var(--red-2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: background-position .25s ease, transform .06s ease;
      white-space: nowrap;
    }

    .counter-btn:hover {
      background-position: left bottom;
    }

    .counter-btn:active {
      transform: translateY(1px);
    }

    @media (max-width: 720px) {
      .page {
        padding: 18px 10px;
      }

      .card-header h1 {
        font-size: 18px;
      }

      .card-body {
        padding: 14px;
      }

      .grid-2 {
        grid-template-columns: 1fr;
      }

      .site-header img {
        height: 44px;
      }

      .counter {
        flex-direction: column;
        align-items: stretch;
      }

      .counter-btn {
        width: 100%;
      }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h1>Configuración del torno</h1>
      </div>

      <div class="card-body">

        <!-- CONTADOR (nuevo) -->
        <div class="counter">
          <div class="counter-left">
            <div class="counter-title">Aperturas totales</div>
            <div class="counter-value" id="aperturasTotales">—</div>
            <div class="counter-sub" id="aperturasInfo">Actualizando…</div>
          </div>

          <button type="button" class="counter-btn" id="btnResetAperturas">
            Reiniciar contador
          </button>
        </div>

        <div class="msg" id="cfgMsg">{{CFG_MSG}}</div>

        <form method="POST" action="/config_save">
          <label>Nombre de la placa</label>
          <input name="deviceId" id="deviceId" value="{{DEVICE_ID}}" maxlength="5" inputmode="numeric"
            autocomplete="off" autocapitalize="characters" spellcheck="false" pattern="ME[0-9]{3}"
            title="Formato: ME seguido de 3 números (ej: ME001)" />


          <div class="grid-2" style="margin-top: 6px;">
            <div class="field">
              <label>Sentido</label>
              <select name="sentido">
                <option {{SENTIDO_ENTRADA_SEL}}>Entrada</option>
                <option {{SENTIDO_SALIDA_SEL}}>Salida</option>
              </select>
            </div>

            <div class="field">
              <label>Modo de red</label>
              <select name="modoRed">
                <option value="0" {{MODO_DHCP_SEL}}>DHCP</option>
                <option value="1" {{MODO_ESTATICA_SEL}}>IP estática</option>
              </select>
            </div>
          </div>

          <label>IP</label>
          <input name="ip" id="ip" value="{{IP}}" maxlength="15" inputmode="decimal" autocomplete="off"
            spellcheck="false" placeholder="192.168.1.191" pattern="^(\d{1,3}\.){3}\d{1,3}$"
            title="Formato IPv4: n.n.n.n" />


          <div class="grid-2">
            <div class="field">
              <label>Gateway</label>
              <input name="gw" id="gw" value="{{GATEWAY}}" maxlength="15" inputmode="decimal" autocomplete="off"
                spellcheck="false" placeholder="192.168.1.250" pattern="^(\d{1,3}\.){3}\d{1,3}$"
                title="Formato IPv4: n.n.n.n" />

            </div>
            <div class="field">
              <label>Subnet</label>
              <input name="mask" id="subnet" value="{{SUBNET}}" maxlength="15" inputmode="decimal" autocomplete="off"
                spellcheck="false" placeholder="255.255.255.0" pattern="^(\d{1,3}\.){3}\d{1,3}$"
                title="Formato IPv4: n.n.n.n" />

            </div>
          </div>

          <div class="grid-2">
            <div class="field">
              <label>DNS 1</label>
              <input name="dns1" id="dns1" value="{{DNS1}}" maxlength="15" inputmode="decimal" autocomplete="off"
                spellcheck="false" placeholder="8.8.8.8" pattern="^(\d{1,3}\.){3}\d{1,3}$"
                title="Formato IPv4: n.n.n.n" />

            </div>
            <div class="field">
              <label>DNS 2</label>
              <input name="dns2" id="dns2" value="{{DNS2}}" maxlength="15" inputmode="decimal" autocomplete="off"
                spellcheck="false" placeholder="4.4.4.4" pattern="^(\d{1,3}\.){3}\d{1,3}$"
                title="Formato IPv4: n.n.n.n" />

            </div>
          </div>

          <label>URL del servidor</label>
          <input name="urlBase" id="urlBase" value="{{URLBASE}}" maxlength="100" autocomplete="off" spellcheck="false"
            inputmode="url" pattern="^https?://.{1,}$"
            title="Debe empezar por http:// o https:// y tener máximo 100 caracteres" />

          <div class="actions">
            <button type="submit">Guardar</button>
            <a class="btn" href="/menu">Volver</a>
          </div>

          <div class="note">
            Nota: al guardar, el dispositivo reinicia para aplicar los cambios de red y backend.
          </div>
        </form>
      </div>
    </div>
  </div>

  <script>

    function enforceDeviceId(el) {
      let v = (el.value || "").toUpperCase();

      // Quitar todo lo que no sea alfanumérico primero
      v = v.replace(/[^A-Z0-9]/g, "");

      // Forzar prefijo "ME"
      if (!v.startsWith("ME")) {
        // Si el usuario empieza metiendo números, los movemos detrás del ME
        v = "ME" + v.replace(/[^0-9]/g, "");
      }

      // Después de ME: solo dígitos
      const tail = v.slice(2).replace(/[^0-9]/g, "").slice(0, 3);

      el.value = ("ME" + tail).slice(0, 5);
    }

    const deviceId = document.getElementById("deviceId");
    if (deviceId) {
      deviceId.addEventListener("input", () => enforceDeviceId(deviceId));
      // Normalizar al cargar por si viene algo raro
      enforceDeviceId(deviceId);
    }

    function enforceIPv4Chars(el) {
      let v = (el.value || "");
      // Solo dígitos y puntos
      v = v.replace(/[^0-9.]/g, "");
      // No permitir dos puntos seguidos
      v = v.replace(/\.{2,}/g, ".");
      // Máximo 15 chars (xxx.xxx.xxx.xxx)
      v = v.slice(0, 15);
      el.value = v;
    }

    ["ip", "gw", "mask", "dns1", "dns2"].forEach(id => {
      const el = document.getElementById(id);
      if (!el) return;
      el.addEventListener("input", () => enforceIPv4Chars(el));
      enforceIPv4Chars(el);
    });

    const urlBase = document.getElementById("urlBase");
    if (urlBase) {
      urlBase.addEventListener("input", () => {
        urlBase.value = (urlBase.value || "").slice(0, 100);
      });
      urlBase.addEventListener("blur", () => {
        urlBase.value = (urlBase.value || "").trim().slice(0, 100);
      });
    }

    // Lógica tipo "aperturasTotalesPrev": solo pintar si cambia
    let aperturasPrev = null;

    function setInfo(txt) {
      const el = document.getElementById('aperturasInfo');
      if (el) el.textContent = txt;
    }

    async function cargarAperturas() {
      try {
        const r = await fetch('/stats', { cache: 'no-store' });
        if (!r.ok) throw new Error('HTTP ' + r.status);
        const j = await r.json();

        const v = (j && typeof j.aperturasTotales !== 'undefined') ? Number(j.aperturasTotales) : null;

        if (v === null || Number.isNaN(v)) {
          setInfo('Sin datos');
          return;
        }

        if (aperturasPrev === null || v !== aperturasPrev) {
          document.getElementById('aperturasTotales').textContent = String(v);
          aperturasPrev = v;
          setInfo('Actualizado');
        } else {
          setInfo('Sin cambios');
        }
      } catch (e) {
        setInfo('Error de conexión');
      }
    }

    async function resetAperturas() {
      if (!confirm('¿Seguro que quieres reiniciar el contador de aperturas?')) return;

      try {
        const r = await fetch('/reset_aperturas', { method: 'POST' });
        if (!r.ok) throw new Error('HTTP ' + r.status);

        aperturasPrev = null; // forzar refresco inmediato
        await cargarAperturas();
        alert('Contador reiniciado.');
      } catch (e) {
        alert('No se pudo reiniciar el contador.');
      }
    }

    document.getElementById('btnResetAperturas').addEventListener('click', resetAperturas);

    // Primera carga + refresco periódico (ajusta el intervalo a tu gusto)
    cargarAperturas();
    setInterval(cargarAperturas, 1000); // 1s: rápido y simple
  </script>
</body>

</html>)HTML";

static const char HTML_FICHEROS[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">

<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Actualizar sistema de ficheros</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root {
      --dark: #292734;
      --dark2: #1f1e28;
      --red: #eb3a3a;
      --red2: #ff3a2d;
      --bg: #fafafa;
      --card: #fff;
      --line: #e6e6ea;
      --muted: #6b6a75;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--dark);
      font-family: "Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing: antialiased;
    }

    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 22px 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      border-bottom: 1px solid var(--line);
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .page {
      min-height: calc(100vh - 96px);
      display: flex;
      justify-content: center;
      align-items: flex-start;
      padding: 28px 14px;
    }

    .card {
      width: 100%;
      max-width: 760px;
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, .08);
      overflow: hidden;
    }

    .card-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 18px;
      color: #fff;
    }

    .card-header h2 {
      margin: 0;
      font-size: 20px;
      font-weight: 900;
    }

    .card-body { padding: 18px; }

    p {
      margin: 10px 0 14px 0;
      color: var(--muted);
      line-height: 1.45;
    }

    code {
      background: #f2f2f6;
      padding: 2px 6px;
      border-radius: 6px;
      border: 1px solid var(--line);
    }

    input[type="file"] {
      width: 100%;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
    }

    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 14px;
    }

    input[type="submit"], .btn {
      border: none;
      border-radius: 10px;
      padding: 12px 16px;
      font-weight: 900;
      letter-spacing: .3px;
      cursor: pointer;
      text-decoration: none;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
    }

    input[type="submit"] {
      color: #fff;
      width: 100%;
      background: linear-gradient(to right, var(--red2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: background-position .25s ease, transform .06s ease;
    }

    input[type="submit"]:hover { background-position: left bottom; }
    input[type="submit"]:active { transform: translateY(1px); }

    .btn {
      width: 100%;
      color: var(--dark);
      background: #fff;
      border: 1px solid var(--line);
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    .btn:hover {
      border-color: rgba(41, 39, 52, .35);
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
    }

    @media (max-width:720px) {
      .page { padding: 18px 10px; }
      .card-body { padding: 14px; }
      .card-header h2 { font-size: 18px; }
      .site-header img { height: 44px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2>Actualizar sistema de ficheros</h2>
      </div>
      <div class="card-body">
        <p>
          Carga de archivos al dispostivo para el sistema de ficheros. Solo se permiten extensiones
          <code>.html</code> y <code>.png</code>. 
        </p>

        <form method="POST" action="/upload_fs" enctype="multipart/form-data">
          <input type="file" name="file" accept=".html,.json" required />
          <div class="actions">
            <input type="submit" value="Subir fichero" />
            <a class="btn" href="/menu">Volver al menú</a>
          </div>
        </form>
      </div>
    </div>
  </div>
</body>

</html>)HTML";

static const char HTML_INDEX[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Acceso al torno</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root{
      --dark:#292734;
      --dark2:#1f1e28;
      --red:#eb3a3a;
      --red2:#ff3a2d;
      --bg:#fafafa;
      --card:#ffffff;
      --line:#e6e6ea;
      --muted:#6b6a75;
    }

    *{ box-sizing:border-box; }

    body{
      margin:0;
      background:var(--bg);
      color:var(--dark);
      font-family:"Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing:antialiased;
      -moz-osx-font-smoothing:grayscale;
    }

    .site-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:22px 16px;
      display:flex;
      justify-content:center;
      align-items:center;
      border-bottom:1px solid var(--line);
    }

    .site-header img{
      height:52px;
      max-width:100%;
      object-fit:contain;
      filter: drop-shadow(0 2px 6px rgba(0,0,0,.25));
    }

    .page{
      min-height:calc(100vh - 96px);
      display:flex;
      justify-content:center;
      align-items:flex-start;
      padding:32px 14px;
    }

    .card{
      width:100%;
      max-width:560px;
      background:var(--card);
      border:1px solid var(--line);
      border-radius:14px;
      box-shadow:0 10px 30px rgba(0,0,0,.08);
      overflow:hidden;
    }

    .card-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:16px 20px;
      color:#ffffff;
    }

    .card-header-text{
      display:flex;
      flex-direction:column;
      line-height:1.1;
    }

    .card-header-text strong{
      font-size:18px;
      font-weight:900;
      letter-spacing:.3px;
    }

    .card-header-text span{
      font-size:13px;
      opacity:.85;
    }

    .card-body{ padding:22px; }

    .msg{
      margin:0 0 14px 0;
      padding:12px;
      border-radius:10px;
      border:1px solid var(--line);
      background:#ffffff;
      font-size:14px;
    }

    label{
      display:block;
      margin:10px 0 6px 0;
      font-weight:800;
      font-size:14px;
    }

    input[type="password"]{
      width:100%;
      padding:12px;
      border-radius:10px;
      border:1px solid var(--line);
      outline:none;
      font-size:14px;
      transition:border-color .15s, box-shadow .15s;
    }

    input[type="password"]:focus{
      border-color:rgba(235,58,58,.8);
      box-shadow:0 0 0 4px rgba(235,58,58,.12);
    }

    .actions{ margin-top:16px; }

    input[type="submit"]{
      border:none;
      border-radius:10px;
      padding:12px 16px;
      font-weight:900;
      letter-spacing:.3px;
      cursor:pointer;
      color:#ffffff;
      width:100%;
      background: linear-gradient(to right,var(--red2) 50%,var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition:background-position .25s ease, transform .06s ease;
    }

    input[type="submit"]:hover{ background-position:left bottom; }
    input[type="submit"]:active{ transform:translateY(1px); }

    .hint{
      margin-top:14px;
      color:var(--muted);
      font-size:13px;
    }

    @media (max-width:720px){
      .site-header img{ height:44px; }
      .card-body{ padding:18px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">

      <div class="card-header">
        <div class="card-header-text">
          <strong>Acceso al torno</strong>
          <span>Panel de administración</span>
        </div>
      </div>

      <div class="card-body">
        <div class="msg">pError</div>

        <form action="/submit" method="POST">
          <label for="password">Contraseña</label>
          <input type="password" id="password" name="password" required />
          <div class="actions">
            <input type="submit" value="Acceder" />
          </div>
        </form>

        <div class="hint">
          Introduce la credencial maestra para acceder al menú.
        </div>

      </div>
    </div>
  </div>
</body>
</html>)HTML";

static const char HTML_LOGS[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Logs</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root {
      --dark: #292734;
      --dark2: #1f1e28;
      --red: #eb3a3a;
      --red2: #ff3a2d;
      --bg: #fafafa;
      --card: #fff;
      --line: #e6e6ea;
      --muted: #6b6a75;
      --ok: #1e9e59;
      --warn: #b35a00;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--dark);
      font-family: "Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing: antialiased;
    }

    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 22px 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      border-bottom: 1px solid var(--line);
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .page {
      min-height: calc(100vh - 96px);
      display: flex;
      justify-content: center;
      align-items: flex-start;
      padding: 28px 14px;
    }

    .card {
      width: 100%;
      max-width: 980px;
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, .08);
      overflow: hidden;
    }

    .card-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 18px;
      color: #fff;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
      flex-wrap: wrap;
    }

    .card-header h2 {
      margin: 0;
      font-size: 20px;
      font-weight: 900;
    }

    .status {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      font-weight: 900;
      font-size: 13px;
      letter-spacing: .2px;
    }

    .dot {
      width: 10px;
      height: 10px;
      border-radius: 99px;
      background: var(--warn);
      box-shadow: 0 0 0 4px rgba(179, 90, 0, .12);
    }

    .dot.ok {
      background: var(--ok);
      box-shadow: 0 0 0 4px rgba(30, 158, 89, .14);
    }

    .card-body { padding: 18px; }

    p {
      margin: 10px 0 12px 0;
      color: var(--muted);
      line-height: 1.45;
    }

    .pill {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      border: 1px solid var(--line);
      background: #f2f2f6;
      color: var(--dark);
      font-weight: 900;
      font-size: 13px;
    }

    .toolbar {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
      margin: 12px 0 14px 0;
    }

    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      min-width: 160px;
    }

    label {
      font-size: 12px;
      color: var(--muted);
      font-weight: 900;
      letter-spacing: .2px;
    }

    select, input[type="number"] {
      width: 100%;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
      color: var(--dark);
      outline: none;
    }

    select:focus, input[type="number"]:focus {
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
      border-color: rgba(41, 39, 52, .35);
    }

    .console {
      width: 100%;
      height: 62vh;
      border: 1px solid var(--line);
      border-radius: 14px;
      background: #0f0f14;
      color: #f4f4f6;
      padding: 12px;
      overflow: auto;
      white-space: pre-wrap;
      font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
      font-size: 12px;
      line-height: 1.35;
    }

    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 14px;
    }

    .btn, button {
      border: none;
      border-radius: 10px;
      padding: 12px 16px;
      font-weight: 900;
      letter-spacing: .3px;
      cursor: pointer;
      text-decoration: none;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
    }

    .btn {
      color: var(--dark);
      background: #fff;
      border: 1px solid var(--line);
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    .btn:hover {
      border-color: rgba(41, 39, 52, .35);
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
    }

    .btn-primary {
      color: #fff;
      background: linear-gradient(to right, var(--red2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: background-position .25s ease, transform .06s ease;
      border: none;
    }

    .btn-primary:hover { background-position: left bottom; }
    .btn-primary:active { transform: translateY(1px); }

    .btn-danger {
      color: #fff;
      border: none;
      background: linear-gradient(90deg, var(--red2) 0%, var(--red) 60%, #b81818 100%);
    }

    .btn-danger:hover { box-shadow: 0 0 0 4px rgba(235, 58, 58, .12); }

    .hint {
      margin-top: 10px;
      font-size: 12px;
      color: var(--muted);
    }

    @media (max-width:720px) {
      .page { padding: 18px 10px; }
      .card-body { padding: 14px; }
      .card-header h2 { font-size: 18px; }
      .site-header img { height: 44px; }
      .console { height: 58vh; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2>Logs en vivo</h2>
        <div class="status">
          <span class="dot" id="dot"></span>
          <span id="st">Conectando...</span>
          <span class="pill" id="last">id: 0</span>
        </div>
      </div>

      <div class="card-body">
        <p>
          Este visor muestra eventos y logs en tiempo real del dispositivo los cuales no se guardan logs en flash.
          Si no hay logs, revisa que estés logueado y que el dispositivo esté funcionando correctamente.
        </p>

        <div class="toolbar">
          <div class="field" style="min-width: 220px;">
            <label>Filtro por prefijo</label>
            <select id="flt">
              <option value="">Todos</option>
              <option value="[HTTP]">[HTTP]</option>
              <option value="[API]">[API]</option>
              <option value="[GM65]">[GM65]</option>
              <option value="[IO]">[IO]</option>
              <option value="[NET]">[NET]</option>
            </select>
          </div>

          <div class="field" style="min-width: 180px;">
            <label>Intervalo refresco (ms)</label>
            <input id="iv" type="number" min="200" step="100" value="800">
          </div>

          <div class="field" style="min-width: 160px;">
            <label>Acción</label>
            <button class="btn btn-primary" type="button" onclick="applyInterval()">Aplicar</button>
          </div>
        </div>

        <div class="console" id="box"></div>

        <div class="actions">
          <button class="btn" type="button" onclick="clearBox()">Limpiar pantalla</button>
          <button class="btn btn-danger" type="button" onclick="togglePause()" id="pauseBtn">Pausar</button>
          <a class="btn" href="/menu">Volver al menú</a>
        </div>

        <div class="hint">
          Consejo: si quieres ver solo lo último, usa “Limpiar pantalla” y deja el filtro en “Todos”.
        </div>
      </div>
    </div>
  </div>

  <script>
    let since = 0;
    let paused = false;
    let timer = null;

    const box = document.getElementById('box');
    const st = document.getElementById('st');
    const dot = document.getElementById('dot');
    const last = document.getElementById('last');
    const flt = document.getElementById('flt');
    const iv = document.getElementById('iv');

    function setOk(ok) {
      if (ok) dot.classList.add('ok');
      else dot.classList.remove('ok');
    }

    function clearBox() { box.textContent = ''; }

    function togglePause() {
      paused = !paused;
      document.getElementById('pauseBtn').textContent = paused ? 'Reanudar' : 'Pausar';
    }

    function applyInterval() {
      const ms = Math.max(200, parseInt(iv.value || '800', 10));
      if (timer) clearInterval(timer);
      timer = setInterval(tick, ms);
      tick();
    }

    async function tick() {
      if (paused) return;

      try {
        const r = await fetch('/logs_data?since=' + since, { cache: 'no-store' });

        if (r.status === 401) { window.location.href = "/"; return; }
        if (!r.ok) { setOk(false); st.textContent = 'HTTP ' + r.status; return; }

        const j = await r.json();
        setOk(true);

        st.textContent = 'Conectado';
        last.textContent = 'id: ' + (j.next || since);

        const pref = flt.value || '';
        if (j.items && j.items.length) {
          for (const it of j.items) {
            const line = '[' + it.id + '] ' + it.msg + '\n';
            if (!pref || (it.msg && it.msg.startsWith(pref))) {
              box.textContent += line;
            }
            since = it.id;
          }
          box.scrollTop = box.scrollHeight;
        }
      } catch (e) {
        setOk(false);
        st.textContent = 'Error de comunicación';
      }
    }

    applyInterval();
  </script>
</body>
</html>)HTML";

static const char HTML_MENU[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Menú Principal</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root{
      --dark:#292734;
      --dark2:#1f1e28;
      --red:#eb3a3a;
      --red2:#ff3a2d;
      --bg:#fafafa;
      --card:#ffffff;
      --line:#e6e6ea;
      --muted:#6b6a75;
    }

    *{ box-sizing:border-box; }

    body{
      margin:0;
      background:var(--bg);
      color:var(--dark);
      font-family:"Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing:antialiased;
    }

    .site-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:22px 16px;
      display:flex;
      justify-content:center;
      align-items:center;
      border-bottom:1px solid var(--line);
    }

    .site-header img{
      height:52px;
      max-width:100%;
      object-fit:contain;
      filter: drop-shadow(0 2px 6px rgba(0,0,0,.25));
    }

    .page{
      min-height:calc(100vh - 96px);
      display:flex;
      justify-content:center;
      align-items:flex-start;
      padding:28px 14px;
    }

    .card{
      width:100%;
      max-width:720px;
      background:var(--card);
      border:1px solid var(--line);
      border-radius:14px;
      box-shadow:0 10px 30px rgba(0,0,0,.08);
      overflow:hidden;
    }

    .card-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:18px;
      color:#ffffff;
    }

    .card-header h2{
      margin:0;
      font-size:20px;
      font-weight:900;
      letter-spacing:.3px;
    }

    .card-body{ padding:18px; }

    p{ margin:8px 0 16px 0; color:var(--muted); }

    ul{
      list-style:none;
      padding:0;
      margin:0;
      display:grid;
      grid-template-columns:1fr 1fr;
      gap:12px;
    }

    a{
      display:flex;
      align-items:center;
      justify-content:center;
      text-decoration:none;
      border-radius:12px;
      padding:14px;
      font-weight:900;
      letter-spacing:.2px;
      border:1px solid var(--line);
      background:#ffffff;
      color:var(--dark);
      box-shadow:0 2px 10px rgba(0,0,0,.05);
      transition: transform .08s ease, box-shadow .15s ease, border-color .15s ease;
      min-height:52px;
      text-align:center;
    }

    a:hover{
      transform:translateY(-1px);
      border-color:rgba(41,39,52,.35);
      box-shadow:0 10px 20px rgba(0,0,0,.08);
    }

    a:active{ transform:translateY(0); }

    .logout{
      margin-top:14px;
      width:100%;
      color:#ffffff;
      border:none;
      background: linear-gradient(to right,var(--red2) 50%,var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition:background-position .25s ease, transform .06s ease;
      box-shadow:none;
    }

    .logout:hover{ background-position:left bottom; }
    .logout:active{ transform:translateY(1px); }

    @media (max-width:720px){
      .page{ padding:18px 10px; }
      .card-body{ padding:14px; }
      .card-header h2{ font-size:18px; }
      ul{ grid-template-columns:1fr; }
      .site-header img{ height:44px; }
    }
  </style>

  <script>
    const INACTIVITY_TIMEOUT_MS = 300000;
    let timeoutId;

    function resetTimer(){
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => { window.location.href = "/logout"; }, INACTIVITY_TIMEOUT_MS);
    }

    const events = ['mousemove','keypress','click','scroll','touchstart','keydown'];
    events.forEach(e => document.addEventListener(e, resetTimer, false));

    window.onload = resetTimer;

    document.addEventListener("visibilitychange", () => {
      if (document.hidden) clearTimeout(timeoutId);
      else resetTimer();
    });
  </script>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2>Menú principal</h2>
      </div>

      <div class="card-body">
        <p>Elige una opción:</p>

        <ul>
          <li><a href="/config">Configuración del torno</a></li>
          <li><a href="/upload_firmware">Actualizar firmware</a></li>
          <li><a href="/upload_fs">Actualizar sistema de ficheros</a></li>
          <li><a href="/logs">Ver logs</a></li>
          <li><a href="/reiniciar">Reiniciar dispositivo</a></li>
        </ul>

        <a class="logout" href="/logout">Cerrar sesión</a>
      </div>
    </div>
  </div>
</body>
</html>)HTML";

static const char HTML_REINICIAR[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Reiniciar dispositivo</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root {
      --dark: #292734;
      --dark2: #1f1e28;
      --red: #eb3a3a;
      --red2: #ff3a2d;
      --bg: #fafafa;
      --card: #fff;
      --line: #e6e6ea;
      --muted: #6b6a75;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--dark);
      font-family: "Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing: antialiased;
    }

    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 22px 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      border-bottom: 1px solid var(--line);
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .page {
      min-height: calc(100vh - 96px);
      display: flex;
      justify-content: center;
      align-items: flex-start;
      padding: 28px 14px;
    }

    .card {
      width: 100%;
      max-width: 720px;
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, .08);
      overflow: hidden;
    }

    .card-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 18px;
      color: #fff;
    }

    .card-header h2 {
      margin: 0;
      font-size: 20px;
      font-weight: 900;
    }

    .card-body { padding: 18px; }

    p {
      margin: 10px 0 14px 0;
      color: var(--muted);
      line-height: 1.45;
    }

    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 14px;
    }

    button, a.btn {
      border: none;
      border-radius: 10px;
      padding: 12px 16px;
      font-weight: 900;
      letter-spacing: .3px;
      cursor: pointer;
      text-decoration: none;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
    }

    button.reboot {
      color: #fff;
      background: linear-gradient(90deg, var(--red2) 0%, var(--red) 60%, #b81818 100%);
      transition: transform .06s ease, box-shadow .15s ease;
    }

    button.reboot:hover { box-shadow: 0 0 0 4px rgba(235, 58, 58, .12); }
    button.reboot:active { transform: translateY(1px); }

    a.btn {
      color: var(--dark);
      background: #fff;
      border: 1px solid var(--line);
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    a.btn:hover {
      border-color: rgba(41, 39, 52, .35);
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
    }

    @media (max-width:720px) {
      .page { padding: 18px 10px; }
      .card-body { padding: 14px; }
      .card-header h2 { font-size: 18px; }
      .site-header img { height: 44px; }
    }
  </style>

  <script>
    const INACTIVITY_TIMEOUT_MS = 300000;
    let timeoutId;

    function resetTimer() {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => window.location.href = "/logout", INACTIVITY_TIMEOUT_MS);
    }

    const events = ['mousemove', 'keypress', 'click', 'scroll', 'touchstart', 'keydown'];
    events.forEach(ev => document.addEventListener(ev, resetTimer, false));
    window.onload = resetTimer;

    document.addEventListener("visibilitychange", () => {
      if (document.hidden) clearTimeout(timeoutId);
      else resetTimer();
    });

    function confirmAndReboot() {
      if (confirm("¿Estás seguro de que quieres reiniciar el dispositivo? Esto puede tardar unos segundos.")) {
        fetch('/reiniciar_dispositivo', { method: 'POST' })
          .then(r => r.ok ? alert('Reiniciando. Recarga la página en unos segundos.') : alert('Error al intentar reiniciar.'))
          .catch(() => alert('Error de conexión al intentar reiniciar.'));
      }
    }
  </script>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2>Reiniciar dispositivo</h2>
      </div>
      <div class="card-body">
        <p>Haz clic en el botón para reiniciar el equipo. Útil para aplicar cambios o resolver incidencias.</p>
        <div class="actions">
          <button class="reboot" type="button" onclick="confirmAndReboot()">Reiniciar ahora</button>
          <a class="btn" href="/menu">Volver al menú</a>
        </div>
      </div>
    </div>
  </div>
</body>
</html>)HTML";

static const char HTML_STATUS[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{{TITLE}}</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root{
      --dark:#292734;
      --dark2:#1f1e28;
      --red:#eb3a3a;
      --red2:#ff3a2d;
      --bg:#fafafa;
      --card:#ffffff;
      --line:#e6e6ea;
      --muted:#6b6a75;
      --ok:#2ea44f;
      --ok2:#1e6f33;
      --info:#3b82f6;
      --info2:#1d4ed8;
      --err:#eb3a3a;
      --err2:#9b1c1c;
    }

    *{ box-sizing:border-box; }

    body{
      margin:0;
      background:var(--bg);
      color:var(--dark);
      font-family:"Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing:antialiased;
      -moz-osx-font-smoothing:grayscale;
    }

    .site-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:22px 16px;
      display:flex;
      justify-content:center;
      align-items:center;
      border-bottom:1px solid var(--line);
    }

    .site-header img{
      height:52px;
      max-width:100%;
      object-fit:contain;
      filter: drop-shadow(0 2px 6px rgba(0,0,0,.25));
    }

    .page{
      min-height:calc(100vh - 96px);
      display:flex;
      justify-content:center;
      align-items:flex-start;
      padding:28px 14px;
    }

    .card{
      width:100%;
      max-width:720px;
      background:var(--card);
      border:1px solid var(--line);
      border-radius:14px;
      box-shadow:0 10px 30px rgba(0,0,0,.08);
      overflow:hidden;
    }

    .card-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:18px;
      color:#ffffff;
    }

    .card-header h2{
      margin:0;
      font-size:20px;
      font-weight:900;
      letter-spacing:.3px;
    }

    .card-body{ padding:18px; }

    .badge{
      display:inline-flex;
      align-items:center;
      gap:8px;
      padding:6px 10px;
      border-radius:999px;
      border:1px solid var(--line);
      background:#f2f2f6;
      font-weight:900;
      font-size:12px;
      letter-spacing:.2px;
      text-transform:uppercase;
      margin-bottom:12px;
      user-select:none;
    }

    .dot{
      width:10px;
      height:10px;
      border-radius:999px;
      display:inline-block;
    }

    .badge.success{ border-color: rgba(46,164,79,.35); background: rgba(46,164,79,.08); color: var(--ok2); }
    .badge.success .dot{ background: var(--ok); }

    .badge.info{ border-color: rgba(59,130,246,.35); background: rgba(59,130,246,.08); color: var(--info2); }
    .badge.info .dot{ background: var(--info); }

    .badge.error{ border-color: rgba(235,58,58,.35); background: rgba(235,58,58,.08); color: var(--err2); }
    .badge.error .dot{ background: var(--err); }

    .msg{
      margin:0 0 14px 0;
      padding:12px;
      border-radius:10px;
      border:1px solid var(--line);
      background:#ffffff;
      font-size:14px;
      line-height:1.45;
      word-break:break-word;
    }

    .actions{
      display:flex;
      gap:10px;
      flex-wrap:wrap;
      margin-top:14px;
    }

    a.btn{
      display:inline-flex;
      align-items:center;
      justify-content:center;
      width:100%;
      border-radius:10px;
      padding:12px 16px;
      font-weight:900;
      letter-spacing:.3px;
      text-decoration:none;
      border:1px solid var(--line);
      color:var(--dark);
      background:#ffffff;
      transition:border-color .15s ease, box-shadow .15s ease;
    }

    a.btn:hover{
      border-color:rgba(41,39,52,.35);
      box-shadow:0 0 0 4px rgba(41,39,52,.08);
    }

    .hint{
      margin-top:12px;
      color:var(--muted);
      font-size:13px;
      line-height:1.35;
    }

    @media (max-width:720px){
      .page{ padding:18px 10px; }
      .card-body{ padding:14px; }
      .card-header h2{ font-size:18px; }
      .site-header img{ height:44px; }
    }
  </style>

  <script>
    (function(){
      var url = "{{AUTO_REDIRECT_URL}}";
      var ms  = parseInt("{{AUTO_REDIRECT_MS}}", 10);
      if (url && url !== "0" && !isNaN(ms) && ms > 0) {
        setTimeout(function(){ window.location.href = url; }, ms);
      }
    })();
  </script>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2>{{TITLE}}</h2>
      </div>

      <div class="card-body">
        <div class="badge {{KIND}}">
          <span class="dot"></span>
          <span>{{KIND}}</span>
        </div>

        <div class="msg">{{MESSAGE}}</div>

        <div class="actions">
          <a class="btn" href="{{BACK_URL}}">{{BACK_TEXT}}</a>
        </div>

        <div class="hint">{{REDIRECT_HINT}}</div>
      </div>
    </div>
  </div>
</body>
</html>)HTML";

static const char HTML_UPLOAD[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Cargar Firmware ESP32</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root {
      --dark: #292734;
      --dark2: #1f1e28;
      --red: #eb3a3a;
      --red2: #ff3a2d;
      --bg: #fafafa;
      --card: #fff;
      --line: #e6e6ea;
      --muted: #6b6a75;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--dark);
      font-family: "Titillium Web", Arial, sans-serif;
      -webkit-font-smoothing: antialiased;
    }

    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 22px 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      border-bottom: 1px solid var(--line);
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .page {
      min-height: calc(100vh - 96px);
      display: flex;
      justify-content: center;
      align-items: flex-start;
      padding: 28px 14px;
    }

    .card {
      width: 100%;
      max-width: 760px;
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, .08);
      overflow: hidden;
    }

    .card-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark2) 33%, var(--red) 66%, var(--red2) 100%);
      padding: 18px;
      color: #fff;
    }

    .card-header h2 {
      margin: 0;
      font-size: 20px;
      font-weight: 900;
    }

    .card-body { padding: 18px; }

    p {
      margin: 10px 0 12px 0;
      color: var(--muted);
      line-height: 1.45;
    }

    .pill {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      border: 1px solid var(--line);
      background: #f2f2f6;
      color: var(--dark);
      font-weight: 900;
      font-size: 13px;
    }

    input[type="file"] {
      width: 100%;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
    }

    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 14px;
    }

    .btn, input[type="submit"] {
      border: none;
      border-radius: 10px;
      padding: 12px 16px;
      font-weight: 900;
      letter-spacing: .3px;
      cursor: pointer;
      text-decoration: none;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
    }

    input[type="submit"] {
      color: #fff;
      background: linear-gradient(to right, var(--red2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: background-position .25s ease, transform .06s ease;
    }

    input[type="submit"]:hover { background-position: left bottom; }
    input[type="submit"]:active { transform: translateY(1px); }

    .btn {
      color: var(--dark);
      background: #fff;
      border: 1px solid var(--line);
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    .btn:hover {
      border-color: rgba(41, 39, 52, .35);
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
    }

    .btn-danger {
      color: #fff;
      border: none;
      background: linear-gradient(90deg, var(--red2) 0%, var(--red) 60%, #b81818 100%);
    }

    .btn-danger:hover { box-shadow: 0 0 0 4px rgba(235, 58, 58, .12); }

    @media (max-width:720px) {
      .page { padding: 18px 10px; }
      .card-body { padding: 14px; }
      .card-header h2 { font-size: 18px; }
      .site-header img { height: 44px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2>Actualizar firmware</h2>
      </div>

      <div class="card-body">
        <p>Versión actual del firmware: <span class="pill">VERSION_FIRMWARE</span></p>
        <p>Seleccione el archivo binario para actualizar el firmware.</p>

        <form method="POST" action="/upload_firmware" enctype="multipart/form-data">
          <input type="file" name="firmware" required />
          <div class="actions">
            <input type="submit" value="Actualizar firmware" />
            <button class="btn btn-danger" type="button" onclick="
              fetch('/download_firmware')
                .then(response => {
                  if (response.ok) alert('Descarga iniciada, el dispositivo se reiniciará en breve.');
                  else response.text().then(text => alert('Error al iniciar descarga: ' + text));
                })
                .catch(error => alert('Error de comunicación: ' + error));
            ">Descargar firmware desde servidor</button>
            <a class="btn" href="/menu">Volver al menú</a>
          </div>
        </form>
      </div>
    </div>
  </div>
</body>
</html>)HTML";


// ============================================================================
// 2) Infra: escribir desde PROGMEM a LittleFS sin copiar a RAM
// ============================================================================

struct PageDef {
  const char* path;
  const char* contentP;   // PROGMEM
};

static size_t progmemStrLen(const char* p) {
  size_t n = 0;
  while (pgm_read_byte(p + n) != 0) n++;
  return n;
}

static bool writeFileFromProgmem(fs::FS& fs, const char* path, const char* contentP)
{
  // Sobrescribe SIEMPRE
  if (fs.exists(path)) {
    fs.remove(path);
  }

  File f = fs.open(path, FILE_WRITE);
  if (!f) {
    Serial.printf("[FS] ERROR open %s\n", path);
    return false;
  }

  const size_t len = progmemStrLen(contentP);

  // Escritura en chunks para ir rápido
  const size_t CHUNK = 512;
  uint8_t buf[CHUNK];

  size_t off = 0;
  while (off < len) {
    size_t n = len - off;
    if (n > CHUNK) n = CHUNK;

    for (size_t i = 0; i < n; i++) {
      buf[i] = (uint8_t)pgm_read_byte(contentP + off + i);
    }

    size_t w = f.write(buf, n);
    if (w != n) {
      Serial.printf("[FS] ERROR write %s (w=%u n=%u)\n", path, (unsigned)w, (unsigned)n);
      f.close();
      return false;
    }
    off += n;
  }

  f.close();
  Serial.printf("[FS] OK %s (%u bytes)\n", path, (unsigned)len);
  return true;
}


// ============================================================================
// 3) Tabla de páginas: paths en LittleFS
//    Ajusta el path si tu proyecto usa otros nombres.
// ============================================================================

static const PageDef pages[] = {
  { "/config.html",          HTML_CONFIG },
  { "/ficheros.html",        HTML_FICHEROS },        // “Actualizar sistema de ficheros”
  { "/index.html",           HTML_INDEX },           // si tú sirves "/" desde otro, cambia el nombre
  { "/logs.html",            HTML_LOGS },
  { "/menu.html",            HTML_MENU },
  { "/reiniciar.html",       HTML_REINICIAR },
  { "/status.html",          HTML_STATUS },          // plantilla para redirectStatus()
  { "/upload.html",          HTML_UPLOAD },
};


// ============================================================================
// 4) Función pública: crea/sobrescribe todos los HTML en LittleFS
// ============================================================================

bool ensureWebPagesInLittleFS(bool formatOnFail)
{
  if (!LittleFS.begin(formatOnFail)) {
    Serial.println("[FS] ERROR LittleFS.begin()");
    return false;
  }

  bool okAll = true;
  for (size_t i = 0; i < sizeof(pages)/sizeof(pages[0]); i++) {
    const bool ok = writeFileFromProgmem(LittleFS, pages[i].path, pages[i].contentP);
    okAll = okAll && ok;
  }

  return okAll;
}
