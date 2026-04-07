#include "ficheros.hpp"

// ============================================================================
// 1) HTMLs en PROGMEM (UNO por fichero)
//    NOTA: Mantén los placeholders {{...}} tal cual (los remplazas en runtime).
// ============================================================================

//============== HTML_INDEX (Login) ==============//============== HTML_INDEX (Login) ==============
// ========================== HTML_INDEX (Acceso al torno) =========================
static const char HTML_INDEX[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title-head">Acceso al torno</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root{
      --dark:#292734; --dark2:#1f1e28; --red:#eb3a3a; --red2:#ff3a2d;
      --bg:#fafafa; --card:#ffffff; --line:#e6e6ea; --muted:#6b6a75;
    }
    *{ box-sizing:border-box; }
    body{ margin:0; background:var(--bg); color:var(--dark); font-family:"Titillium Web", Arial, sans-serif; -webkit-font-smoothing:antialiased; }

    .site-header{
      background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%);
      padding:22px 16px; display:flex; justify-content:center; align-items:center; position: relative;
      border-bottom:1px solid var(--line);
      min-height: 96px;
    }
    .site-header img{ height:52px; max-width:100%; object-fit:contain; filter: drop-shadow(0 2px 6px rgba(0,0,0,.25)); }

    .lang-select { position: absolute; right: 20px; display: flex; gap: 8px; }
    .lang-btn { background: rgba(255,255,255,0.2); border: 1px solid #fff; color: #fff; padding: 4px 8px; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: bold; transition: all 0.2s; }
    .lang-btn.active { background: #fff; color: var(--dark); }

    .page{ display:flex; justify-content:center; align-items:flex-start; padding:32px 14px; min-height:calc(100vh - 96px); }
    .card{ width:100%; max-width:560px; background:var(--card); border:1px solid var(--line); border-radius:14px; box-shadow:0 10px 30px rgba(0,0,0,.08); overflow:hidden; }
    .card-header{ background: linear-gradient(90deg,var(--dark) 0%,var(--dark2) 33%,var(--red) 66%,var(--red2) 100%); padding:16px 20px; color:#ffffff; }
    .card-body{ padding:22px; }

    .msg { display: none; margin-bottom: 14px; padding: 12px; border-radius: 10px; font-size: 14px; text-align: center; }
    .msg.error { display: block; border: 1px solid var(--red); background: rgba(235,58,58,0.08); color: var(--red); font-weight: bold; }

    label{ display:block; margin:10px 0 6px 0; font-weight:800; font-size:14px; }
    
    /* Contenedor para el ojo dentro del input */
    .password-wrapper {
      position: relative;
      display: flex;
      align-items: center;
      width: 100%;
    }
    .password-wrapper input {
      width: 100%;
      padding: 12px;
      padding-right: 45px; /* Espacio para el icono */
      border-radius: 10px;
      border: 1px solid var(--line);
      outline: none;
      font-size: 14px;
      transition: all 0.2s;
    }
    .password-wrapper input:focus {
      border-color: var(--red);
      box-shadow: 0 0 0 4px rgba(235,58,58,0.12);
    }
    
    .toggle-password {
      position: absolute;
      right: 12px;
      background: none !important;
      border: none;
      padding: 0;
      cursor: pointer;
      display: flex;
      align-items: center;
      z-index: 10;
      opacity: 0.6;
    }
    .toggle-password:hover { opacity: 1; }
    .toggle-password svg { width: 22px; height: 22px; fill: var(--muted); }

    input[type="submit"]{ border:none; border-radius:10px; padding:12px 16px; font-weight:900; cursor:pointer; color:#ffffff; width:100%; background: var(--dark); margin-top:16px; transition: all 0.2s; }
    input[type="submit"]:hover{ background: var(--red); }
    
    .hint{ margin-top:14px; color:var(--muted); font-size:13px; }

    @media (max-width:720px){
      .site-header { flex-direction: column; padding: 10px; min-height: auto; }
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header img { height: 44px; }
    }
  </style>
</head>
<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">   
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <strong style="font-size: 18px;">Acceso al torno</strong><br>
        <small id="txt-header-sub">Panel de administración</small>
      </div>
      <div class="card-body">
        <div id="error-box" class="msg">{{ERROR_MSG}}</div>

        <form action="/submit" method="POST">
          <label id="txt-label-pass" for="password">Contraseña</label>
          <div class="password-wrapper">
            <input type="password" id="password" name="password" required />
            <button type="button" class="toggle-password" onclick="togglePassView()">
              <svg id="eye-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                <path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>
              </svg>
            </button>
          </div>
          <input type="submit" id="btn-submit" value="Acceder" />
        </form>
        <div class="hint" id="txt-hint">Introduce la credencial maestra.</div>
      </div>
    </div>
  </div>

<script>
  const translations = {
    es: {
      title: "Acceso al torno",
      headerSub: "Panel de administración",
      labelPass: "Contraseña",
      btnSubmit: "Acceder",
      hint: "Introduce la credencial maestra para acceder al menú.",
      authError: "Contraseña incorrecta."
    },
    en: {
      title: "Gate Access",
      headerSub: "Administration Panel",
      labelPass: "Password",
      btnSubmit: "Login",
      hint: "Enter the master credential to access the menu.",
      authError: "Invalid password."
    }
  };

  function togglePassView() {
    const passInp = document.getElementById("password");
    const eyeIcon = document.getElementById("eye-icon");
    if (passInp.type === "password") {
      passInp.type = "text";
      eyeIcon.innerHTML = '<path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.82l2.92 2.92c1.51-1.26 2.7-2.89 3.44-4.74-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/>';
    } else {
      passInp.type = "password";
      eyeIcon.innerHTML = '<path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>';
    }
  }

  function setLang(lang) {
    localStorage.setItem('selectedLang', lang);
    const t = translations[lang];
    document.title = t.title;
    document.getElementById('txt-header-sub').innerText = t.headerSub;
    document.getElementById('txt-label-pass').innerText = t.labelPass;
    document.getElementById('btn-submit').value = t.btnSubmit;
    document.getElementById('txt-hint').innerText = t.hint;
    
    document.getElementById('btn-es').classList.toggle('active', lang === 'es');
    document.getElementById('btn-en').classList.toggle('active', lang === 'en');
    document.documentElement.lang = lang;

    const eb = document.getElementById('error-box');
    if (eb.classList.contains('error')) {
      eb.innerText = t.authError;
    }
  }

  window.onload = () => {
    const savedLang = localStorage.getItem('selectedLang') || (navigator.language.startsWith('en') ? 'en' : 'es');
    setLang(savedLang);

    const eb = document.getElementById('error-box');
    const content = eb.innerText.trim();

    if (content === "AUTH_ERROR_ID") {
      eb.innerText = translations[savedLang].authError;
      eb.classList.add('error');
    } else {
      eb.classList.remove('error');
      eb.style.display = 'none';
      eb.innerText = "";
    }
  };
</script>
</body>
</html>)HTML";
//===============================================================

//============== HTML_STATUS (Página de estado con redirección) ==============
static const char HTML_MENU[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Menú Principal</title>

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
      position: relative;
    }

    .site-header img{
      height:52px;
      max-width:100%;
      object-fit:contain;
      filter: drop-shadow(0 2px 6px rgba(0,0,0,.25));
    }

    /* Selector de Idioma en Header */
    .lang-select {
      position: absolute;
      right: 20px;
      display: flex;
      gap: 8px;
    }

    .lang-btn {
      background: rgba(255,255,255,0.2);
      border: 1px solid #fff;
      color: #fff;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }

    .lang-btn.active {
      background: #fff;
      color: var(--dark);
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
      grid-template-columns: 1fr 1fr;
      gap:12px;
    }

    ul li:last-child:nth-child(odd) {
      grid-column: 1 / span 2;
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
      width: 100%;
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
      display: flex;
      align-items: center;
      justify-content: center;
      text-decoration: none;
      border-radius: 12px;
      padding: 14px;
      font-weight: 900;
      background: linear-gradient(to right,var(--red2) 50%,var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition:background-position .25s ease, transform .06s ease;
    }

    .logout:hover{ background-position:left bottom; }
    .logout:active{ transform:translateY(1px); }

    @media (max-width:720px){
      .page{ padding:18px 10px; }
      .card-body{ padding:14px; }
      .card-header h2{ font-size:18px; }
      ul{ grid-template-columns:1fr; }
      ul li:last-child:nth-child(odd) { grid-column: span 1; }
      .site-header img{ height:44px; }
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2 id="txt-header">Menú principal</h2>
      </div>

      <div class="card-body">
        <p id="txt-sub">Elige una opción:</p>

        <ul>
          <li><a href="/config" id="m-config">Configuración del torno</a></li>          
          <li><a href="/logs" id="m-logs">Ver logs</a></li>
          <li><a href="/actions" id="m-actions">Acciones del torno</a></li>
          <!-- <li><a href="/params" id="m-params">Parámetros del torno</a></li> -->
          <li><a href="/upload_firmware" id="m-firmware">Actualizar firmware</a></li>
          <li><a href="/upload_fs" id="m-fs">Actualizar sistema de ficheros</a></li>
          <li><a href="/reiniciar" id="m-reboot">Reiniciar dispositivo</a></li>
        </ul>

        <a class="logout" href="/logout" id="btn-logout">Cerrar sesión</a>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Menú Principal",
        header: "Menú principal",
        sub: "Elige una opción:",
        config: "Configuración del torno",
        logs: "Ver logs",
        actions: "Acciones del torno",
        params: "Parámetros del torno",
        firmware: "Actualizar firmware",
        fs: "Actualizar sistema de ficheros",
        reboot: "Reiniciar dispositivo",
        logout: "Cerrar sesión"
      },
      en: {
        title: "Main Menu",
        header: "Main Menu",
        sub: "Choose an option:",
        config: "Gate Configuration",
        logs: "View Logs",
        actions: "Gate Actions",
        params: "Gate Parameters",
        firmware: "Update Firmware",
        fs: "Update File System",
        reboot: "Reboot Device",
        logout: "Logout"
      }
    };

    function setLang(lang) {
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      
      document.title = t.title;
      document.getElementById('txt-header').innerText = t.header;
      document.getElementById('txt-sub').innerText = t.sub;
      document.getElementById('m-config').innerText = t.config;
      document.getElementById('m-logs').innerText = t.logs;
      document.getElementById('m-actions').innerText = t.actions;
      document.getElementById('m-params').innerText = t.params;
      document.getElementById('m-firmware').innerText = t.firmware;
      document.getElementById('m-fs').innerText = t.fs;
      document.getElementById('m-reboot').innerText = t.reboot;
      document.getElementById('btn-logout').innerText = t.logout;
      
      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
      
      document.documentElement.lang = lang;
    }

    // --- Lógica de Inactividad ---
    const INACTIVITY_TIMEOUT_MS = 300000;
    let timeoutId;

    function resetTimer(){
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => { window.location.href = "/logout"; }, INACTIVITY_TIMEOUT_MS);
    }

    const events = ['mousemove','keypress','click','scroll','touchstart','keydown'];
    events.forEach(e => document.addEventListener(e, resetTimer, false));

    window.onload = () => {
      const savedLang = localStorage.getItem('selectedLang') || 'es';
      setLang(savedLang);
      resetTimer();
    };

    document.addEventListener("visibilitychange", () => {
      if (document.hidden) clearTimeout(timeoutId);
      else resetTimer();
    });
  </script>
</body>
</html>)HTML";
//===============================================================
// ========================== HTML_CONFIG (Configuración del torno) =========================
static const char HTML_CONFIG[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Configuración del torno</title>
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
      --readonly-bg: #f2f2f5;
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
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark-2) 33%, var(--red) 66%, var(--red-2) 100%);
      padding: 22px 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      border-bottom: 1px solid var(--line);
      position: relative;
    }
    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }
    .lang-select {
      position: absolute;
      right: 20px;
      display: flex;
      gap: 8px;
    }
    .lang-btn {
      background: rgba(255,255,255,0.2);
      border: 1px solid #fff;
      color: #fff;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }
    .lang-btn.active { background: #fff; color: var(--dark); }

    .page {
      min-height: calc(100vh - 96px);
      display: flex;
      justify-content: center;
      align-items: flex-start;
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
      padding: 18px 24px;
      color: #fff;
    }
    .card-header h1 { margin: 0; font-size: 20px; font-weight: 700; letter-spacing: .2px; }
    .card-body { padding: 24px; }
    .msg {
      margin-bottom: 20px;
      padding: 12px 16px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
      color: var(--dark);
      font-size: 14px;
      text-align: center;
    }
    .section-title {
      font-size: 13px;
      font-weight: 800;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 1px;
      margin: 20px 0 10px 0;
      padding-bottom: 5px;
      border-bottom: 1px solid var(--line);
    }
    label { display: block; margin: 12px 0 6px 0; font-weight: 700; font-size: 14px; color: var(--dark); }
    input, select {
      width: 100%;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fff;
      color: var(--dark);
      outline: none;
      font-size: 14px;
      transition: all .2s ease;
    }
    input:focus, select:focus { border-color: var(--red); box-shadow: 0 0 0 4px rgba(235, 58, 58, .12); }
    input[readonly] { background-color: var(--readonly-bg); cursor: not-allowed; color: var(--muted); }
    
    .password-field-container {
      position: relative;
      display: flex;
      align-items: center;
      width: 100%;
    }
    .password-field-container input { padding-right: 45px; }
    .toggle-password {
      position: absolute;
      right: 15px;
      background: none !important;
      border: none;
      padding: 0;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 10;
      width: 24px;
      height: 24px;
      opacity: 0.6;
      transition: opacity 0.2s;
    }
    .toggle-password:hover { opacity: 1; }
    .toggle-password svg { width: 20px; height: 20px; fill: var(--muted); }

    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
    .actions { display: flex; gap: 12px; margin-top: 30px; }
    button {
      flex: 1; border: none; border-radius: 10px; padding: 14px; font-weight: 800; cursor: pointer; color: #fff;
      background: linear-gradient(to right, var(--red-2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: all .3s ease;
    }
    button:hover { background-position: left bottom; }
    a.btn {
      flex: 1; display: inline-flex; align-items: center; justify-content: center; border-radius: 10px; padding: 14px;
      font-weight: 800; text-decoration: none; border: 1px solid var(--line); color: var(--dark); background: #fff;
      transition: all .2s ease;
    }
    .note {
      margin-top: 20px; padding: 12px; background: #fff9f9; border-radius: 10px; color: var(--muted);
      font-size: 13px; line-height: 1.4; border: 1px dashed var(--red);
    }
    
    /* ESTILOS NUEVOS PARA EL ESCANEO WIFI */
    .scan-wrapper {
      display: flex;
      gap: 10px;
    }
    .scan-wrapper select {
      flex: 1;
    }
    .scan-wrapper input {
      flex: 1;
    }
    #scan-status {
      font-size: 11px;
      color: var(--red);
      font-weight: normal;
      margin-left: 8px;
    }

    @media (max-width: 600px) {
      .grid-2 { grid-template-columns: 1fr; }
      .actions { flex-direction: column; }
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
      .scan-wrapper { flex-direction: column; gap: 6px; }
    }
  </style>
</head>
<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h1 id="txt-h1">Configuración del Torno</h1>
      </div>
      <div class="card-body">
        <div class="msg" id="cfgMsg">{{CFG_MSG}}</div>
        <form method="POST" action="/config_save">
          
          <div class="section-title" id="sec-hw">Identificación y Hardware</div>
          <div class="grid-2">
            <div class="field">
              <label id="lbl-devid">Nombre de la placa (ID)</label>
              <input name="deviceId" id="deviceId" value="{{DEVICE_ID}}" maxlength="6" pattern="QRD[0-9]{3}" required />
            </div>
            <div class="field">
              <label id="lbl-mac">Dirección MAC</label>
              <input value="{{MAC_ADDRESS}}" readonly />
            </div>
          </div>
          <div class="grid-2">
            <div class="field">
              <label id="lbl-model">Tipo de Pasillo</label>
              <select name="modoPasillo">
                <option value="0" {{MOD_VEGA_SEL}}>VEGA</option>
                <option value="1" {{MOD_CANOPU_SEL}}>CANOPU</option>
                <option value="2" {{MOD_ARTURUS_SEL}}>ARTURUS</option>
              </select>
            </div>
            <div class="field"></div>
          </div>

          <div class="section-title" id="sec-ops">Modos de Operación</div>
          <div class="grid-2">
            <div class="field">
              <label id="lbl-open">Modo de Apertura</label>
              <select name="modoApertura">
                <option value="0" id="opt-rs485" {{MODO_RS485_SEL}}>RS485</option>
                <option value="1" id="opt-reles" {{MODO_RELES_SEL}}>Relés</option>
              </select>
            </div>
            <div class="field">
              <label id="lbl-sentido">Sentido de Apertura (Entrada)</label>
              <select name="sentidoApertura">
                <option value="0" id="opt-left" {{SENT_LEFT_SEL}}>Izquierda</option>
                <option value="1" id="opt-right" {{SENT_RIGHT_SEL}}>Derecha</option>
              </select>
            </div>
          </div>

          <div class="section-title" id="sec-net">Configuración de Red</div>
          <div class="grid-2">
            <div class="field">
              <label id="lbl-conn">Tipo de Conexión</label>
              <select name="conexionRed" id="conexionRed" onchange="updateWiFiFields()">
                <option value="0" {{CON_WIFI_SEL}}>WiFi</option>
                <option value="1" {{CON_ETH_SEL}}>Ethernet</option>
              </select>
            </div>
            <div class="field">
              <label id="lbl-netmode">Modo de Red</label>
              <select name="modoRed" id="modoRed" onchange="updateIPFieldsStatus()">
                <option value="0" id="opt-dhcp" {{MODO_DHCP_SEL}}>DHCP (Automático)</option>
                <option value="1" id="opt-static" {{MODO_ESTATICA_SEL}}>IP Estática</option>
              </select>
            </div>
          </div>

          <div id="wifiFields" style="display:none;" class="grid-2">
            <div class="field">
              <label id="lbl-ssid">SSID WiFi <span id="scan-status"></span></label>
              <div class="scan-wrapper">
                <select id="wifiSelect" onchange="seleccionarRed(this.value)">
                  <option value="">Buscando redes...</option>
                </select>
                <input name="wifiSSID" id="wifiSSID" value="{{WIFI_SSID}}" maxlength="32" placeholder="O escribe el SSID..." />
              </div>
            </div>
            <div class="field">
              <label id="lbl-pass">Contraseña WiFi</label>
              <div class="password-field-container">
                <input name="wifiPass" id="wifiPass" type="password" value="{{WIFI_PASS}}" maxlength="64" />
                <button type="button" class="toggle-password" onclick="togglePassView()" id="btn-toggle-eye">
                  <svg id="eye-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                    <path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>
                  </svg>
                </button>
              </div>
            </div>
          </div>

          <div class="grid-2">
            <div class="field">
              <label id="lbl-ip">IP del Dispositivo</label>
              <input name="ip" id="ip" value="{{IP}}" />
            </div>
            <div class="field">
              <label id="lbl-gw">Puerta de Enlace (Gateway)</label>
              <input name="gw" id="gw" value="{{GATEWAY}}" />
            </div>
          </div>

          <div class="grid-2">
            <div class="field">
              <label id="lbl-mask">Máscara de Subred</label>
              <input name="mask" id="mask" value="{{SUBNET}}" />
            </div>
            <div class="field">
              <label id="lbl-dns1">DNS 1</label>
              <input name="dns1" id="dns1" value="{{DNS1}}" />
            </div>
          </div>

          <div class="grid-2">
            <div class="field">
              <label id="lbl-dns2">DNS 2</label>
              <input name="dns2" id="dns2" value="{{DNS2}}" />
            </div>
            <div class="field"></div>
          </div>

          <div class="section-title" id="sec-api">Comunicación Externa (API)</div>
          <label id="lbl-url-base">URL Principal</label>
          <input name="urlBase" id="urlBase" value="{{URL_BASE}}" maxlength="200" required />

          <div id="tabletSection" {{TABLET_CLASS}}>
            <label id="lbl-url-tab">URL Tablet</label>
            <input name="tabletURL" id="tabletURL" value="{{URL_TABLET}}" maxlength="200" />
          </div>

          <label id="lbl-url-upd">URL de Actualización</label>
          <input name="urlActualiza" id="urlActualiza" value="{{URL_ACTUALIZA}}" maxlength="200" />

          <div class="actions">
            <button type="submit" id="btn-save">Guardar y Reiniciar</button>
            <a class="btn" href="/menu" id="btn-back">Volver al Menú</a>
          </div>

          <div class="note" id="note-reboot">
            <strong id="note-reboot-bold">Atención:</strong> El dispositivo se reiniciará automáticamente para aplicar los cambios.
          </div>
        </form>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Configuración del torno",
        h1: "Configuración del Torno",
        hw: "Identificación y Hardware",
        devid: "Nombre de la placa (ID)",
        mac: "Dirección MAC",
        model: "Tipo de Pasillo",
        ops: "Modos de Operación",
        open: "Modo de Apertura",
        sentido: "Sentido de Apertura (Entrada)",
        net: "Configuración de Red",
        conn: "Tipo de Conexión",
        netmode: "Modo de Red",
        ssid: "SSID WiFi",
        pass: "Contraseña WiFi",
        ip: "IP del Dispositivo",
        gw: "Puerta de Enlace (Gateway)",
        mask: "Máscara de Subred",
        dns1: "DNS 1",
        dns2: "DNS 2",
        api: "Comunicación Externa (API)",
        urlbase: "URL Principal",
        urltab: "URL Tablet",
        urlupd: "URL de Actualización",
        save: "Guardar y Reiniciar",
        back: "Volver al Menú",
        note: "<strong>Atención:</strong> El dispositivo se reiniciará automáticamente para aplicar los cambios de red y conexión.",
        opt_rs485: "RS485", opt_reles: "Relés",
        opt_left: "Izquierda", opt_right: "Derecha",
        opt_dhcp: "DHCP (Automático)", opt_static: "IP Estática",
        // Nuevas traducciones para escaneo
        scan_status: "(Escaneando...)",
        scan_error: "(Error al escanear)",
        scan_select: "Selecciona una red..."
      },
      en: {
        title: "Gate Configuration",
        h1: "Gate Configuration",
        hw: "Identification & Hardware",
        devid: "Board Name (ID)",
        mac: "MAC Address",
        model: "Turnstile Type",
        ops: "Operation Modes",
        open: "Opening Mode",
        sentido: "Opening Direction (Entry)",
        net: "Network Configuration",
        conn: "Connection Type",
        netmode: "Network Mode",
        ssid: "WiFi SSID",
        pass: "WiFi Password",
        ip: "Device IP",
        gw: "Gateway",
        mask: "Subnet Mask",
        dns1: "DNS 1",
        dns2: "DNS 2",
        api: "External Communication (API)",
        urlbase: "Primary URL",
        urltab: "Tablet URL",
        urlupd: "Update URL",
        save: "Save & Reboot",
        back: "Back to Menu",
        note: "<strong>Warning:</strong> The device will automatically reboot to apply network and connection changes.",
        opt_rs485: "RS485", opt_reles: "Relays",
        opt_left: "Left", opt_right: "Right",
        opt_dhcp: "DHCP (Automatic)", opt_static: "Static IP",
        // Nuevas traducciones para escaneo
        scan_status: "(Scanning...)",
        scan_error: "(Scan error)",
        scan_select: "Select a network..."
      }
    };

    function setLang(lang) {
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      
      document.title = t.title;
      document.getElementById('txt-h1').innerText = t.h1;
      document.getElementById('sec-hw').innerText = t.hw;
      document.getElementById('lbl-devid').innerText = t.devid;
      document.getElementById('lbl-mac').innerText = t.mac;
      document.getElementById('lbl-model').innerText = t.model;
      
      document.getElementById('sec-ops').innerText = t.ops;
      document.getElementById('lbl-open').innerText = t.open;
      document.getElementById('lbl-sentido').innerText = t.sentido;
      
      document.getElementById('sec-net').innerText = t.net;
      document.getElementById('lbl-conn').innerText = t.conn;
      document.getElementById('lbl-netmode').innerText = t.netmode;
      
      // Aseguramos mantener el span de estado de escaneo
      const scanStatusHtml = document.getElementById('scan-status').outerHTML;
      document.getElementById('lbl-ssid').innerHTML = t.ssid + " " + scanStatusHtml;
      
      document.getElementById('lbl-pass').innerText = t.pass;
      document.getElementById('lbl-ip').innerText = t.ip;
      document.getElementById('lbl-gw').innerText = t.gw;
      document.getElementById('lbl-mask').innerText = t.mask;
      document.getElementById('lbl-dns1').innerText = t.dns1;
      document.getElementById('lbl-dns2').innerText = t.dns2;
      
      document.getElementById('sec-api').innerText = t.api;
      document.getElementById('lbl-url-base').innerText = t.urlbase;
      if(document.getElementById('lbl-url-tab')) document.getElementById('lbl-url-tab').innerText = t.urltab;
      document.getElementById('lbl-url-upd').innerText = t.urlupd;
      
      document.getElementById('btn-save').innerText = t.save;
      document.getElementById('btn-back').innerText = t.back;
      document.getElementById('note-reboot').innerHTML = t.note;

      // Traducción de opciones internas de los select
      document.getElementById('opt-rs485').text = t.opt_rs485;
      document.getElementById('opt-reles').text = t.opt_reles;
      document.getElementById('opt-left').text = t.opt_left;
      document.getElementById('opt-right').text = t.opt_right;
      document.getElementById('opt-dhcp').text = t.opt_dhcp;
      document.getElementById('opt-static').text = t.opt_static;
      
      // Traducción por defecto del select WiFi si está vacío o en estado inicial
      const wifiSel = document.getElementById('wifiSelect');
      if(wifiSel && wifiSel.options.length > 0 && wifiSel.options[0].value === "") {
         wifiSel.options[0].text = t.scan_select;
      }
      
      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
    }

    function togglePassView() {
      const passInp = document.getElementById("wifiPass");
      const eyeIcon = document.getElementById("eye-icon");
      if (passInp.type === "password") {
        passInp.type = "text";
        eyeIcon.innerHTML = '<path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.82l2.92 2.92c1.51-1.26 2.7-2.89 3.44-4.74-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/>';
      } else {
        passInp.type = "password";
        eyeIcon.innerHTML = '<path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>';
      }
    }

    function enforceDeviceId(el) {
      let v = (el.value || "").toUpperCase().replace(/[^A-Z0-9]/g, "");
      if (!v.startsWith("QRD")) v = "QRD" + v.replace(/[^0-9]/g, "");
      const tail = v.slice(3).replace(/[^0-9]/g, "").slice(0, 3);
      el.value = ("QRD" + tail).slice(0, 6);
    }

    function updateIPFieldsStatus() {
      const modoRedSel = document.getElementById("modoRed");
      const isDHCP = modoRedSel.value === "0";
      ["ip", "gw", "mask", "dns1", "dns2"].forEach(id => {
        const el = document.getElementById(id);
        if(el) {
          el.readOnly = isDHCP;
          el.style.backgroundColor = isDHCP ? "var(--readonly-bg)" : "#fff";
        }
      });
    }

    // --- NUEVA LÓGICA DE ESCANEO WIFI ---
    let scanInterval = null;

    async function fetchNetworks() {
      const statusEl = document.getElementById('scan-status');
      const lang = localStorage.getItem('selectedLang') || 'es';
      const t = translations[lang];
      
      statusEl.innerText = t.scan_status;
      
      try {
        const res = await fetch('/scan_wifi');
        const nets = await res.json();
        const sel = document.getElementById('wifiSelect');
        
        sel.innerHTML = `<option value="">${t.scan_select}</option>`;
        nets.forEach(n => {
          sel.innerHTML += `<option value="${n.ssid}">${n.ssid} (${n.rssi} dBm)</option>`;
        });
        
        statusEl.innerText = "";
      } catch(e) {
        statusEl.innerText = t.scan_error;
      }
    }

    function seleccionarRed(ssid) {
      if(ssid) {
        document.getElementById('wifiSSID').value = ssid;
        document.getElementById('wifiPass').focus();
      }
    }

    function updateWiFiFields() {
      const connType = document.getElementById("conexionRed").value;
      const wifiSection = document.getElementById("wifiFields");
      
      if (connType === "0") {
        wifiSection.style.display = "grid";
        fetchNetworks(); 
        if (!scanInterval) scanInterval = setInterval(fetchNetworks, 15000); 
      } else {
        wifiSection.style.display = "none";
        if (scanInterval) {
          clearInterval(scanInterval);
          scanInterval = null;
        }
      }
    }
    // ------------------------------------

    window.onload = () => {
      const savedLang = localStorage.getItem('selectedLang') || 'es';
      setLang(savedLang);
      
      const devInp = document.getElementById("deviceId");
      if(devInp) devInp.addEventListener("input", () => enforceDeviceId(devInp));
      
      updateIPFieldsStatus();
      updateWiFiFields();

      const msgBox = document.getElementById('cfgMsg');
      if (msgBox.innerText.includes("{{") || msgBox.innerText.trim() === "") msgBox.style.display = 'none';
    };
  </script>
</body>
</html>)HTML";
//=======================================================================================
// ========================== HTML_ACTIONS (Acciones del torno) =========================
static const char HTML_ACTIONS[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Control de Torno - Qualica-RD</title>
  <link rel="icon" href="/logo.png" type="image/png" />
  <style>
    :root {
      --dark: #292734; --dark-2: #1f1e28; --red: #eb3a3a; --red-2: #ff3a2d;
      --bg: #fafafa; --card: #ffffff; --line: #e6e6ea; --muted: #6b6a75;
      --yellow: #f1c40f; --readonly-bg: #f2f2f5; --green: #27ae60;
    }
    * { box-sizing: border-box; font-family: "Titillium Web", Arial, sans-serif; }
    body { margin: 0; background: var(--bg); color: var(--dark); -webkit-font-smoothing: antialiased; }
    
    /* Clase para ocultar elementos segun el modoApertura */
    .hidden { display: none !important; }

    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark-2) 33%, var(--red) 66%, var(--red-2) 100%);
      padding: 22px 16px; display: flex; justify-content: center; align-items: center; border-bottom: 1px solid var(--line);
      position: relative;
    }
    .site-header img { height: 52px; filter: drop-shadow(0 2px 6px rgba(0,0,0,.25)); }
    
    .lang-select { position: absolute; right: 20px; display: flex; gap: 8px; }
    .lang-btn {
      background: rgba(255,255,255,0.2); border: 1px solid #fff; color: #fff;
      padding: 4px 8px; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: bold; transition: all 0.2s;
    }
    .lang-btn.active { background: #fff; color: var(--dark); }

    .page { min-height: calc(100vh - 96px); display: flex; justify-content: center; padding: 28px 14px; }
    .card { width: 100%; max-width: 880px; background: var(--card); border: 1px solid var(--line); border-radius: 14px; box-shadow: 0 10px 30px rgba(0,0,0,.08); overflow: hidden; }
    .card-header {      background: linear-gradient(90deg, var(--dark) 0%, var(--dark-2) 33%, var(--red) 66%, var(--red-2) 100%); padding: 18px 24px; color: #fff;}
    .card-header h1 { margin: 0; font-size: 20px; font-weight: 700; }
    .card-body { padding: 24px; }

    .module-header { display: flex; justify-content: space-between; align-items: flex-end; margin: 20px 0 10px 0; padding-bottom: 5px; border-bottom: 1px solid var(--line); }
    .section-title-inline { font-size: 13px; font-weight: 800; color: var(--muted); text-transform: uppercase; letter-spacing: 1px; margin: 0; }
    
    .btn-refresh-small {
      background: var(--dark) !important; color: #fff !important;
      padding: 5px 12px; border-radius: 6px; cursor: pointer; font-size: 10px; font-weight: 800;
      text-transform: uppercase; transition: all 0.2s ease-in-out; width: auto !important; display: inline-block; box-shadow: none;
    }
    .btn-refresh-small:hover { transform: scale(1.1); filter: brightness(1.1); }

    button { 
      border: none; border-radius: 10px; padding: 14px; font-weight: 800; cursor: pointer; color: #fff;
      background: linear-gradient(to right, var(--red-2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: all .2s ease-in-out; text-transform: uppercase; width: 100%;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    button:hover { transform: scale(1.02); box-shadow: 0 5px 15px rgba(0,0,0,0.2); filter: brightness(1.1); }
    button:active { transform: scale(0.96); filter: brightness(0.9); }

    .grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; margin-bottom: 10px; }
    .status-item { padding: 12px; background: var(--readonly-bg); border-radius: 10px; text-align: center; border: 1px solid var(--line); }
    .status-label { display: block; font-size: 11px; font-weight: 800; color: var(--muted); margin-bottom: 4px; }
    .status-val { font-size: 14px; font-weight: 700; color: var(--dark); }

    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
    .counter-box { text-align: center; padding: 15px; border: 1px solid var(--line); border-radius: 12px; }
    .counter-num { font-size: 32px; font-weight: 900; color: var(--red); display: block; margin: 5px 0; }

    .apertura-container { display: flex; flex-direction: column; align-items: center; gap: 15px; margin: 20px 0; }
    .qty-input { width: 100px; padding: 12px; border: 2px solid var(--line); border-radius: 10px; font-size: 20px; font-weight: 900; text-align: center; outline: none; transition: border-color 0.2s; }
    .qty-input:focus { border-color: var(--red); }
    
    .btn-yellow { background: var(--yellow) !important; color: var(--dark) !important; }
    .btn-red { background: var(--red) !important; color: #fff !important; }
    .btn-white { background: #fff !important; color: var(--dark) !important; border: 1px solid var(--line) !important; }
    .btn-green { background: var(--green) !important; color: #fff !important; }
    .btn-group-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
    
    @media (max-width: 600px) {
      .grid-3, .grid-2, .btn-group-3 { grid-template-columns: 1fr; }
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
    }
  </style>
</head>
<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header"><h1 id="txt-h1">Panel de Control Operativo</h1></div>
      <div class="card-body">
        
        <div class="{{RS485_CLASS}}">
          <div class="module-header">
            <p class="section-title-inline" id="sec-info">Información del Torno</p>
            <button class="btn-refresh-small" onclick="updateStatus()" id="btn-refresh">Refrescar ↻</button>
          </div>
          <div class="grid-3">
            <div class="status-item"><span class="status-label" id="lbl-gates">Puertas</span><span id="stGate" class="status-val">---</span></div>
            <div class="status-item"><span class="status-label" id="lbl-fault">Fallo/Avisos</span><span id="stFault" class="status-val">---</span></div>
            <div class="status-item"><span class="status-label" id="lbl-alarm">Seguridad</span><span id="stAlarm" class="status-val">---</span></div>
          </div>
        </div>

        <div class="module-header"><p class="section-title-inline" id="sec-prog">Apertura Programada</p></div>
        <div class="apertura-container">
          <div class="{{RS485_CLASS}}" style="text-align:center;">
             <span class="status-label" id="lbl-qty">NUMERO DE PASOS</span>
             <input type="number" id="p_qty" class="qty-input" value="1" min="1" style="margin-bottom:10px;">
          </div>
          <div class="grid-2" style="width:100%">
            <button onclick="openTorno('r')" id="btn-open-ent">Abrir Entrada</button>
            <button onclick="openTorno('l')" id="btn-open-sal">Abrir Salida</button>
          </div>
        </div>

        <div class="{{RS485_CLASS}}">
          <div class="grid-2">
            <button class="btn-red" onclick="confirmEmergency()" id="btn-emergency">Apertura Emergencia</button>
            <button class="btn-yellow" onclick="execAction('close')" id="btn-close">Cierre de Puertas</button>
          </div>

          <div class="module-header"><p class="section-title-inline" id="sec-restr">Restricciones de Paso</p></div>
          <div class="grid-2">
            <button class="btn-white" style="color:var(--red)" onclick="execAction('forbid_r')" id="btn-forbid-ent">Bloquear Entrada</button>
            <button class="btn-white" style="color:var(--red)" onclick="execAction('forbid_l')" id="btn-forbid-sal">Bloquear Salida</button>
          </div>
          <button class="btn-green" style="margin-top:10px" onclick="execAction('unrestrict')" id="btn-unrestr">Quitar Restricciones</button>

          <div style="margin-top:40px; border-top: 1px solid var(--line); padding-top:20px">
            <button class="btn-red" onclick="confirmFullReset()" id="btn-full-reset">Reiniciar Placa de Control</button>
          </div>
        </div>

        <div style="margin-top:10px;">
          <button class="btn-white" onclick="location.href='/menu'" id="btn-back">Volver al Menú Principal</button>
        </div>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Control de Torno - Qualica-RD", h1: "Panel de Control Operativo",
        info: "Información del Torno", refresh: "Refrescar ↻",
        gates: "Puertas", fault: "Fallo/Avisos", alarm: "Seguridad",
        resent: "Reset Entrada", ressal: "Reset Salida",
        prog: "Apertura Programada", qty: "NUMERO DE PASOS",
        opent: "Abrir Entrada", opens: "Abrir Salida",
        emer: "Apertura Emergencia", close: "Cierre de Puertas",
        cstop: "Volver a Estado Inicial", restr: "Restricciones de Paso",
        fent: "Bloquear Entrada", fsal: "Bloquear Salida", unrestr: "Quitar Restricciones",
        full: "Reiniciar Placa de Control", back: "Volver al Menú Principal",
        confEmer: "¿Activar apertura permanente de emergencia?",
        confFull: "¿Seguro que desea REINICIAR el sistema de control?"
      },
      en: {
        title: "Turnstile Control - Qualica-RD", h1: "Operational Control Panel",
        info: "Turnstile Information", refresh: "Refresh ↻",
        gates: "Gates", fault: "Fault/Warnings", alarm: "Security",
        flow: "Passenger Flow", ent: "ENTRIES", sal: "EXITS",
        resent: "Reset Entry", ressal: "Reset Exit",
        prog: "Scheduled Opening", qty: "STEP QUANTITY",
        opent: "Open Entry", opens: "Open Exit",
        emer: "Emergency Opening", close: "Close Gates",
        cont: "Continuous Flow Modes", cent: "Free Entry", call: "Free In/Out", csal: "Free Exit",
        cstop: "Back to Initial State", restr: "Passage Restrictions",
        fent: "Block Entry", fsal: "Block Exit", unrestr: "Clear Restrictions",
        full: "Reset Control Board", back: "Back to Main Menu",
        confEmer: "Activate PERMANENT emergency opening?",
        confFull: "Are you sure you want to REBOOT the control system?"
      }
    };

    let currentLang = 'es';

    function setLang(lang) {
      currentLang = lang;
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      document.getElementById('txt-title').innerText = t.title;
      document.getElementById('txt-h1').innerText = t.h1;
      document.getElementById('sec-info').innerText = t.info;
      document.getElementById('btn-refresh').innerText = t.refresh;
      document.getElementById('lbl-gates').innerText = t.gates;
      document.getElementById('lbl-fault').innerText = t.fault;
      document.getElementById('lbl-alarm').innerText = t.alarm;
      document.getElementById('sec-flow').innerText = t.flow;
      document.getElementById('lbl-ent').innerText = t.ent;
      document.getElementById('lbl-sal').innerText = t.sal;
      document.getElementById('btn-res-ent').innerText = t.resent;
      document.getElementById('btn-res-sal').innerText = t.ressal;
      document.getElementById('sec-prog').innerText = t.prog;
      document.getElementById('lbl-qty').innerText = t.qty;
      document.getElementById('btn-open-ent').innerText = t.opent;
      document.getElementById('btn-open-sal').innerText = t.opens;
      document.getElementById('btn-emergency').innerText = t.emer;
      document.getElementById('btn-close').innerText = t.close;
      document.getElementById('sec-cont').innerText = t.cont;
      document.getElementById('btn-cont-ent').innerText = t.cent;
      document.getElementById('btn-cont-all').innerText = t.call;
      document.getElementById('btn-cont-sal').innerText = t.csal;
      document.getElementById('btn-cont-stop').innerText = t.cstop;
      document.getElementById('sec-restr').innerText = t.restr;
      document.getElementById('btn-forbid-ent').innerText = t.fent;
      document.getElementById('btn-forbid-sal').innerText = t.fsal;
      document.getElementById('btn-unrestr').innerText = t.unrestr;
      document.getElementById('btn-full-reset').innerText = t.full;
      document.getElementById('btn-back').innerText = t.back;

      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
    }

    async function updateStatus() {
    try {
      console.log("Solicitando estado al servidor...");
      const r = await fetch('/get_status_json');
      if (!r.ok) throw new Error("Error en la respuesta del servidor");
      
      const j = await r.json();
      console.log("Datos recibidos:", j);

      // Actualizamos los textos de estado (RS485)
      // Usamos condicionales para evitar errores si los elementos están ocultos
      const elGate = document.getElementById('stGate');
      const elFault = document.getElementById('stFault');
      const elAlarm = document.getElementById('stAlarm');

      if(elGate) elGate.textContent = j.gate_text;
      if(elFault) elFault.textContent = j.fault_text;
      if(elAlarm) elAlarm.textContent = j.alarm_text;

      // Actualizamos los contadores (Común)
      document.getElementById('valEntradas').textContent = j.cnt_der;
      document.getElementById('valSalidas').textContent = j.cnt_izq;

    } catch(e) {
      console.error("Error actualizando el estado:", e);
    }
  }

    function openTorno(dir) {
      const input = document.getElementById('p_qty');
      const n = (input && input.offsetParent !== null) ? (input.value || 1) : 1;
      execAction('open_' + dir, n);
    }

    async function execAction(cmd, p = 1) {
      try {
        await fetch(`/do_action?cmd=${cmd}&p=${p}`);
        setTimeout(updateStatus, 400); 
      } catch(e) {}
    }

    function confirmEmergency() { if(confirm(translations[currentLang].confEmer)) execAction('emergency'); }
    function confirmFullReset() { if(confirm(translations[currentLang].confFull)) execAction('reset_full'); }

    window.onload = () => { 
        const savedLang = localStorage.getItem('selectedLang') || 'es';
        setLang(savedLang);
        setInterval(updateStatus, 3000); 
        updateStatus(); 
    };
  </script>
</body>
</html>)HTML";
//===============================================================

// ========================== HTML_FICHEROS (Actualización del sistema de ficheros) =========================
static const char HTML_FICHEROS[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">

<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Actualizar sistema de ficheros</title>

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
      position: relative;
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .lang-select {
      position: absolute;
      right: 20px;
      display: flex;
      gap: 8px;
    }

    .lang-btn {
      background: rgba(255,255,255,0.2);
      border: 1px solid #fff;
      color: #fff;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }

    .lang-btn.active { background: #fff; color: var(--dark); }

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
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2 id="txt-header">Actualizar sistema de ficheros</h2>
      </div>
      <div class="card-body">
        <p id="txt-desc">
          Carga de archivos al dispositivo para el sistema de ficheros. Solo se permiten extensiones
          <code>.html</code> y <code>.png</code>. 
        </p>

        <form method="POST" action="/upload_fs" enctype="multipart/form-data">
          <input type="file" name="file" accept=".html,.json,.png" required />
          <div class="actions">
            <input type="submit" id="btn-submit" value="Subir fichero" />
            <a class="btn" href="/menu" id="btn-back">Volver al menú</a>
          </div>
        </form>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Actualizar sistema de ficheros",
        header: "Actualizar sistema de ficheros",
        desc: "Carga de archivos al dispositivo para el sistema de ficheros. Solo se permiten extensiones <code>.html</code>, <code>.json</code> y <code>.png</code>.",
        submit: "Subir fichero",
        back: "Volver al menú"
      },
      en: {
        title: "Update File System",
        header: "Update File System",
        desc: "Upload files to the device file system. Only <code>.html</code>, <code>.json</code> and <code>.png</code> extensions are allowed.",
        submit: "Upload file",
        back: "Back to menu"
      }
    };

    function setLang(lang) {
      // Guardamos SIEMPRE para que las otras páginas lo sepan
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      
      document.title = t.title;
      document.getElementById('txt-header').innerText = t.header;
      document.getElementById('txt-desc').innerHTML = t.desc;
      document.getElementById('btn-submit').value = t.submit;
      document.getElementById('btn-back').innerText = t.back;
      
      // Actualizar botones visualmente
      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
      document.documentElement.lang = lang;
    }

    // Al cargar, buscamos lo que guardó el login o el menú
    window.onload = () => {
      const savedLang = localStorage.getItem('selectedLang') || 'es';
      setLang(savedLang);
    };
  </script>
</body>
</html>)HTML";
//===============================================================

// ========================== HTML_LOGS (Visualización de logs) =========================
static const char HTML_LOGS[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Logs</title>

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
      position: relative;
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .lang-select {
      position: absolute;
      right: 20px;
      display: flex;
      gap: 8px;
    }

    .lang-btn {
      background: rgba(255,255,255,0.2);
      border: 1px solid #fff;
      color: #fff;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }

    .lang-btn.active { background: #fff; color: var(--dark); }

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
      box-shadow: 0 10px 30px rgba(0,0,0,.08);
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

    .btn-primary {
      color: #fff;
      background: linear-gradient(to right, var(--red2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: background-position .25s ease, transform .06s ease;
      border: none;
    }

    .btn-danger {
      color: #fff;
      border: none;
      background: linear-gradient(90deg, var(--red2) 0%, var(--red) 60%, #b81818 100%);
    }

    .hint {
      margin-top: 10px;
      font-size: 12px;
      color: var(--muted);
    }

    @media (max-width:720px) {
      .page { padding: 18px 10px; }
      .card-body { padding: 14px; }
      .site-header img { height: 44px; }
      .console { height: 58vh; }
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2 id="txt-header">Logs en vivo</h2>
        <div class="status">
          <span class="dot" id="dot"></span>
          <span id="st">...</span>
          <span class="pill" id="last">id: 0</span>
        </div>
      </div>

      <div class="card-body">
        <p id="txt-desc">
          Este visor muestra eventos y logs en tiempo real del dispositivo los cuales no se guardan logs en flash.
          Si no hay logs, revisa que estés logueado y que el dispositivo esté funcionando correctamente.
        </p>

        <div class="toolbar">
          <div class="field" style="min-width: 220px;">
            <label id="lbl-filter">Filtro por prefijo</label>
            <select id="flt">
              <option value="" id="opt-all">Todos</option>
              <option value="[MAIN]">[MAIN]</option>
              <option value="[HTTP]">[HTTP]</option>
              <option value="[API]">[API]</option>
              <option value="[DSSP3120]">[DSSP3120]</option>
              <option value="[RFID]">[RFID]</option>
              <option value="[IO]">[IO]</option>
              <option value="[NET]">[NET]</option>
            </select>
          </div>

          <div class="field" style="min-width: 180px;">
            <label id="lbl-interval">Intervalo refresco (ms)</label>
            <input id="iv" type="number" min="200" step="100" value="800">
          </div>

          <div class="field" style="min-width: 160px;">
            <label id="lbl-action">Acción</label>
            <button class="btn btn-primary" type="button" onclick="applyInterval()" id="btn-apply">Aplicar</button>
          </div>
        </div>

        <div class="console" id="box"></div>

        <div class="actions">
          <button class="btn" type="button" onclick="clearBox()" id="btn-clear">Limpiar pantalla</button>
          <button class="btn btn-danger" type="button" onclick="togglePause()" id="pauseBtn">Pausar</button>
          <a class="btn" href="/menu" id="btn-back">Volver al menú</a>
        </div>

        <div class="hint" id="txt-hint">
          Consejo: si quieres ver solo lo último, usa “Limpiar pantalla” y deja el filtro en “Todos”.
        </div>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Logs",
        header: "Logs en vivo",
        desc: "Este visor muestra eventos y logs en tiempo real del dispositivo los cuales no se guardan en flash.",
        filter: "Filtro por prefijo",
        all: "Todos",
        interval: "Intervalo refresco (ms)",
        action: "Acción",
        apply: "Aplicar",
        clear: "Limpiar pantalla",
        pause: "Pausar",
        resume: "Reanudar",
        back: "Volver al menú",
        hint: "Consejo: si quieres ver solo lo último, usa “Limpiar pantalla” y deja el filtro en “Todos”.",
        connecting: "Conectando...",
        connected: "Conectado",
        commErr: "Error de comunicación"
      },
      en: {
        title: "Logs",
        header: "Live Logs",
        desc: "This viewer shows real-time events and logs from the device which are not saved in flash memory.",
        filter: "Filter by prefix",
        all: "All",
        interval: "Refresh interval (ms)",
        action: "Action",
        apply: "Apply",
        clear: "Clear screen",
        pause: "Pause",
        resume: "Resume",
        back: "Back to menu",
        hint: "Tip: if you only want to see the latest, use “Clear screen” and leave the filter on “All”.",
        connecting: "Connecting...",
        connected: "Connected",
        commErr: "Communication error"
      }
    };

    let currentLang = 'es';
    let since = 0;
    let paused = false;
    let timer = null;

    const box = document.getElementById('box');
    const st = document.getElementById('st');
    const dot = document.getElementById('dot');
    const last = document.getElementById('last');
    const flt = document.getElementById('flt');
    const iv = document.getElementById('iv');

    function setLang(lang) {
      currentLang = lang;
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      
      document.title = t.title;
      document.getElementById('txt-header').innerText = t.header;
      document.getElementById('txt-desc').innerText = t.desc;
      document.getElementById('lbl-filter').innerText = t.filter;
      document.getElementById('opt-all').innerText = t.all;
      document.getElementById('lbl-interval').innerText = t.interval;
      document.getElementById('lbl-action').innerText = t.action;
      document.getElementById('btn-apply').innerText = t.apply;
      document.getElementById('btn-clear').innerText = t.clear;
      document.getElementById('pauseBtn').innerText = paused ? t.resume : t.pause;
      document.getElementById('btn-back').innerText = t.back;
      document.getElementById('txt-hint').innerText = t.hint;
      
      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
      document.documentElement.lang = lang;
    }

    function setOk(ok) {
      const t = translations[currentLang];
      if (ok) {
        dot.classList.add('ok');
        st.textContent = t.connected;
      } else {
        dot.classList.remove('ok');
        st.textContent = t.commErr;
      }
    }

    function clearBox() { box.textContent = ''; }

    function togglePause() {
      paused = !paused;
      const t = translations[currentLang];
      document.getElementById('pauseBtn').textContent = paused ? t.resume : t.pause;
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
        if (!r.ok) { setOk(false); return; }

        const j = await r.json();
        setOk(true);
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
      }
    }

    window.onload = () => {
      const savedLang = localStorage.getItem('selectedLang') || 'es';
      setLang(savedLang);
      applyInterval();
    };
  </script>
</body>
</html>)HTML";
//===========================================================================================================

//======================= HTML_PARAMS (Visualización y edición de parámetros técnicos) =======================
static const char HTML_PARAMS[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Parámetros Técnicos - Qualica-RD</title>

  <link rel="icon" href="/logo.png" type="image/png" />
  <link rel="apple-touch-icon" href="/logo.png" />

  <style>
    :root {
      --dark: #292734; --dark-2: #1f1e28; --red: #eb3a3a; --red-2: #ff3a2d;
      --bg: #fafafa; --card: #ffffff; --line: #e6e6ea; --muted: #6b6a75;
      --readonly-bg: #f2f2f5; --green: #2ecc71;
    }
    * { box-sizing: border-box; }
    body { margin: 0; background: var(--bg); color: var(--dark); font-family: "Titillium Web", Arial, sans-serif; -webkit-font-smoothing: antialiased; }
    
    .site-header {
      background: linear-gradient(90deg, var(--dark) 0%, var(--dark-2) 33%, var(--red) 66%, var(--red-2) 100%);
      padding: 15px; display: flex; justify-content: center; align-items: center; border-bottom: 1px solid var(--line);
    }
    .site-header img { height: 45px; filter: drop-shadow(0 2px 6px rgba(0,0,0,.25)); }

    .page { padding: 20px; max-width: 1400px; margin: 0 auto; }
    
    .card { background: var(--card); border: 1px solid var(--line); border-radius: 14px; box-shadow: 0 10px 30px rgba(0,0,0,.08); margin-bottom: 20px; overflow: hidden; }
    .card-header { background: var(--dark); padding: 15px 20px; color: #fff; display: flex; justify-content: space-between; align-items: center; }
    .card-header h1 { margin: 0; font-size: 18px; text-transform: uppercase; letter-spacing: 1px; }

    .card-body { padding: 20px; }
    .table-container { overflow-x: auto; margin-bottom: 20px; }
    
    table { width: 100%; border-collapse: collapse; min-width: 1000px; }
    th { text-align: left; background: #f8f9fa; padding: 12px; font-size: 12px; color: var(--muted); text-transform: uppercase; border-bottom: 2px solid var(--line); }
    td { padding: 12px; border-bottom: 1px solid var(--line); font-size: 14px; vertical-align: middle; }
    
    .desc-col { font-size: 12px; color: var(--muted); line-height: 1.3; max-width: 350px; white-space: normal; }
    
    .val-input { width: 100%; padding: 8px; border-radius: 6px; border: 1px solid var(--line); outline: none; background: #fff; font-size: 14px; }
    .val-input:focus { border-color: var(--red); box-shadow: 0 0 0 3px rgba(235, 58, 58, 0.1); }
    
    button {
      border: none; border-radius: 8px; padding: 10px 15px; font-weight: 800; cursor: pointer; color: #fff;
      background: linear-gradient(to right, var(--red-2) 50%, var(--dark) 50%) no-repeat right bottom / 210% 100%;
      transition: all .3s ease; text-transform: uppercase; font-size: 11px;
    }
    button:hover { background-position: left bottom; }
    
    .btn-lang { background: #fff; color: var(--dark); border: 1px solid var(--line); margin-left: 5px; padding: 5px 10px; border-radius: 4px; cursor: pointer; }
    .btn-lang.active { background: var(--red); color: #fff; border-color: var(--red); }

    /* Estilo del botón Volver al Menú (Copiado de Reiniciar) */
    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 14px;
    }

    a.btn-block {
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
      color: var(--dark);
      background: #fff;
      border: 1px solid var(--line);
      transition: border-color .15s ease, box-shadow .15s ease;
    }

    a.btn-block:hover {
      border-color: rgba(41, 39, 52, .35);
      box-shadow: 0 0 0 4px rgba(41, 39, 52, .08);
    }

    @media (max-width:720px) {
      .page { padding: 15px 10px; }
      .card-body { padding: 14px; }
      .site-header img { height: 35px; }
    }
  </style>
</head>
<body>
  <header class="site-header"><img src="/qualicard.png" alt="Qualica-RD"></header>
  
  <div class="page">
    <div class="card">
      <div class="card-header">
        <h1 id="t-title">Cargando...</h1>
        <div>
          <button id="btn-es" class="btn-lang">ES</button>
          <button id="btn-en" class="btn-lang">EN</button>
        </div>
      </div>
      <div class="card-body">
        <p id="t-conf-sub" style="color: var(--muted); font-size: 13px; margin-bottom: 20px;"></p>
        
        <div class="table-container">
          <table id="confTable">
            <thead>
              <tr>
                <th id="th-id">ID</th>
                <th id="th-fn">Función</th>
                <th id="th-desc">Descripción</th>
                <th id="th-range">Rango</th>
                <th id="th-val">Valor Actual</th>
                <th id="th-act">Acción</th>
              </tr>
            </thead>
            <tbody>
              <tr><td colspan="6" style="text-align:center">Cargando parámetros desde el dispositivo...</td></tr>
            </tbody>
          </table>
        </div>

        <div class="actions">
          <a href="/menu" class="btn-block" id="btnBackMenu">VOLVER AL MENÚ</a>
        </div>
      </div>
    </div>
  </div>

  <script>
    const dict = {
      es: {
        title: "Parámetros del Torno",
        sub: "Configuración técnica avanzada vía RS485.",
        btnSave: "Guardar",
        back: "VOLVER AL MENÚ",
        thId: "ID", thFn: "Función", thDesc: "Descripción", thRange: "Rango", thVal: "Valor Actual", thAct: "Acción",
        confirm: "¿Guardar en memoria y reiniciar mecanismo?",
        success: "Parámetro guardado correctamente.",
        errConn: "Error de comunicación con el ESP32."
      },
      en: {
        title: "Gate Parameters",
        sub: "Advanced technical configuration via RS485.",
        btnSave: "Save",
        back: "BACK TO MENU",
        thId: "ID", thFn: "Function", thDesc: "Description", thRange: "Range", thVal: "Current Value", thAct: "Action",
        confirm: "Save to memory and reboot mechanism?",
        success: "Parameter saved successfully.",
        errConn: "Communication error with ESP32."
      }
    };

    const P = [
      {id:0,type:"number",range:[1,99],def:1, name:{es:"Número de máquina",en:"Machine Number"}, desc:{es:"Identificador RS-485 único por equipo.",en:"Unique RS-485 device address."}},
      {id:1,type:"enum",range:[1,4],def:1, name:{es:"Modo de apertura de la puerta",en:"Gate Opening Mode"}, desc:{es:"Lógica de apertura; en paso libre se guía por IR.",en:"Opening logic; free-pass driven by IR direction."},
        options:[{v:1,l:{es:"1 · Modo estándar",en:"1 · Standard mode"}},{v:2,l:{es:"2 · Paso libre IR izquierdo",en:"2 · Left IR free passage"}},{v:3,l:{es:"3 · Paso libre IR derecho",en:"3 · Right IR free passage"}},{v:4,l:{es:"4 · Paso libre IR izquierda y derecha",en:"4 · Left & Right IR free passage"}}]},
      {id:2,type:"number",range:[1,90],def:8, name:{es:"Tiempo de espera de apertura (s)",en:"Gate Opening Wait Time (s)"}, desc:{es:"Segundos de apertura antes de auto-cierre.",en:"Seconds the gate stays open before auto-close."}},
      {id:3,type:"enum",range:[0,9],def:3, name:{es:"Voz apertura izquierda",en:"Left Gate Opening Voice"}, desc:{es:"Locución al abrir hacia la izquierda.",en:"Voice prompt when opening to the left."},
        options:[{v:0,l:{es:"0 · Gracias",en:"0 · Thank you"}},{v:1,l:{es:"1 · Pase por favor",en:"1 · Come in please"}},{v:2,l:{es:"2 · Nos vemos",en:"2 · See you"}},{v:3,l:{es:"3 · Bienvenida",en:"3 · Welcome"}},{v:4,l:{es:"4 · Adiós",en:"4 · Goodbye"}},{v:5,l:{es:"5 · Que tenga un buen día",en:"5 · Have a nice day"}},{v:6,l:{es:"6 · Que tenga un buen viaje",en:"6 · Have a nice trip"}},{v:7,l:{es:"7 · Póngase el casco",en:"7 · Please wear a safety helmet"}},{v:8,l:{es:"8 · Verificado con éxito",en:"8 · Verify success"}},{v:9,l:{es:"9 · Silencio",en:"9 · Mute"}}]},
      {id:4,type:"enum",range:[0,9],def:5, name:{es:"Voz apertura derecha",en:"Right Gate Opening Voice"}, desc:{es:"Locución al abrir hacia la derecha.",en:"Voice prompt when opening to the right."}, options:null},
      {id:5,type:"number",range:[1,15],def:12, name:{es:"Volumen de voz",en:"Voice Volume"}, desc:{es:"Mayor valor = más volumen.",en:"Higher value = louder volume."}},
      {id:6,type:"number",range:[1,25],def:10, name:{es:"Velocidad motor maestro",en:"Master Motor Speed"}, desc:{es:"Evita sobrevelocidad para proteger hardware.",en:"Avoid overspeed to protect hardware."}},
      {id:7,type:"number",range:[1,25],def:10, name:{es:"Velocidad motor esclavo",en:"Slave Motor Speed"}, desc:{es:"Velocidad base del motor esclavo.",en:"Base speed for the slave motor."}},
      {id:8,type:"enum",range:[0,2],def:0, name:{es:"Modo depuración",en:"Debugging Mode"}, desc:{es:"Test; 2 restaura fábrica.",en:"Tests; 2 restores factory."},
        options:[{v:0,l:{es:"0 · Normal",en:"0 · Normal"}},{v:1,l:{es:"1 · Envejecimiento",en:"1 · Aging"}},{v:2,l:{es:"2 · Restaurar fábrica",en:"2 · Restore factory"}}]},
      {id:9,type:"number",range:[1,20],def:10, name:{es:"Rango de desaceleración",en:"Deceleration Range"}, desc:{es:"Mayor valor = frenado más suave.",en:"Higher value = softer deceleration."}},
      {id:10,type:"number",range:[1,9],def:3, name:{es:"Velocidad autocomprobación",en:"Self-test Speed"}, desc:{es:"Velocidad del self-test de arranque.",en:"Startup self-test speed."}},
      {id:11,type:"enum",range:[0,2],def:0, name:{es:"Modo de paso",en:"Passage Mode"}, desc:{es:"Control de flujo.",en:"Flow control."},
        options:[{v:0,l:{es:"0 · Paso suave",en:"0 · Smooth"}},{v:1,l:{es:"1 · Memoria",en:"1 · Memory"}},{v:2,l:{es:"2 · 1 Tarjeta = 1 Persona",en:"2 · 1 Card = 1 Person"}}]},
      {id:12,type:"enum",range:[0,9],def:2, name:{es:"Control de cierre",en:"Gate Closing Control"}, desc:{es:"Estrategia y retardo del cierre.",en:"Closing strategy."},
        options:[{v:0,l:{es:"0 · Tras IR medio",en:"0 · After middle IR"}},{v:1,l:{es:"1 · Tras último IR",en:"1 · After last IR"}},{v:2,l:{es:"2 · Tras último IR + Retardo",en:"2 · After last IR + Delay"}}]},
      {id:13,type:"enum",range:[0,1],def:0, name:{es:"Modo motor único",en:"Single Motor Mode"}, desc:{es:"Topología del mecanismo.",en:"Mechanism topology."},
        options:[{v:0,l:{es:"0 · Doble motor",en:"0 · Dual motor"}},{v:1,l:{es:"1 · Motor único",en:"1 · Single motor"}}]},
      {id:14,type:"enum",range:[0,6],def:4, name:{es:"Idioma",en:"Language"}, desc:{es:"Idioma de locuciones.",en:"Prompt language."},
        options:[{v:0,l:{es:"0 · Chino",en:"0 · Chinese"}},{v:1,l:{es:"1 · Inglés",en:"1 · English"}},{v:4,l:{es:"4 · Español",en:"4 · Spanish"}}]},
      {id:15,type:"enum",range:[0,1],def:1, name:{es:"Antipellizco IR",en:"IR Anti-pinch"}, desc:{es:"Rebote al detectar obstrucción.",en:"Rebound on obstacle."},
        options:[{v:0,l:{es:"0 · Sin rebote",en:"0 · No rebound"}},{v:1,l:{es:"1 · Con rebote",en:"1 · Rebound"}}]},
      {id:16,type:"number",range:[1,9],def:5, name:{es:"Sensibilidad mecánica",en:"Mechanical Sensitivity"}, desc:{es:"Mayor valor = más sensible.",en:"Higher value = more sensitive."}},
      {id:17,type:"enum",range:[0,1],def:1, name:{es:"Entrada inversa",en:"Reverse Entry"}, desc:{es:"Reacción ante paso inverso.",en:"Reverse passage response."},
        options:[{v:0,l:{es:"0 · Solo alarma",en:"0 · Alarm only"}},{v:1,l:{es:"1 · Alarma + Cierre",en:"1 · Alarm + Close"}}]},
      {id:18,type:"enum",range:[0,3],def:0, name:{es:"Tipo de torno",en:"Turnstile Type"}, desc:{es:"Modelo mecánico.",en:"Mechanical model."},
        options:[{v:0,l:{es:"0 · Giratoria estándar",en:"0 · Standard swing"}},{v:1,l:{es:"1 · Cilíndrica",en:"1 · Cylindrical"}},{v:2,l:{es:"2 · Abatible",en:"2 · Flap gate"}}]},
      {id:19,type:"enum",range:[0,2],def:2, name:{es:"Apertura emergencia",en:"Emergency Opening"}, desc:{es:"Corte de energía.",en:"Power loss behavior."},
        options:[{v:0,l:{es:"0 · Izquierda",en:"0 · Left"}},{v:1,l:{es:"1 · Derecha",en:"1 · Right"}},{v:2,l:{es:"2 · Automática",en:"2 · Auto"}}]},
      {id:20,type:"number",range:[1,9],def:5, name:{es:"Resistencia motor",en:"Motor Resistance"}, desc:{es:"Fuerza de empuje.",en:"Push force."}},
      {id:21,type:"enum",range:[0,1],def:1, name:{es:"Alarma intrusión",en:"Intrusion Alarm"}, desc:{es:"Locución ante intrusión.",en:"Intrusion voice."},
        options:[{v:0,l:{es:"0 · OFF",en:"0 · OFF"}},{v:1,l:{es:"1 · ON",en:"1 · ON"}}]},
      {id:22,type:"number",range:[1,9],def:5, name:{es:"Retardo IR",en:"IR Signal Delay"}, desc:{es:"Unidades de 20ms.",en:"20ms units."}},
      {id:23,type:"enum",range:[1,4],def:1, name:{es:"Sentido motores",en:"Motor Direction"}, desc:{es:"Sincroniza hojas.",en:"Sync leaves."},
        options:[{v:1,l:{es:"1 · M Fwd, S Rev",en:"1 · M Fwd, S Rev"}},{v:3,l:{es:"3 · Ambos Fwd",en:"3 · Both Fwd"}}]},
      {id:24,type:"enum",range:[0,3],def:0, name:{es:"Embrague",en:"Clutch"}, desc:{es:"Gestión bloqueo.",en:"Clutch management."},
        options:[{v:0,l:{es:"0 · Auto",en:"0 · Auto"}},{v:1,l:{es:"1 · Sin embrague",en:"1 · No clutch"}}]},
      {id:25,type:"enum",range:[0,4],def:0, name:{es:"Hall motor",en:"Motor Hall Type"}, desc:{es:"Fases hall.",en:"Hall phasing."},
        options:[{v:0,l:{es:"0 · Auto",en:"0 · Auto"}},{v:1,l:{es:"1 · +120°",en:"1 · +120°"}}]},
      {id:26,type:"number",range:[1,9],def:3, name:{es:"Filtro entrada",en:"Input Filter"}, desc:{es:"Unidades de 10ms.",en:"10ms units."}},
      {id:27,type:"enum",range:[0,1],def:0, name:{es:"Tarjeta interior",en:"Card Inside"}, desc:{es:"Validar en canal.",en:"Aisle validation."},
        options:[{v:0,l:{es:"0 · Sí",en:"0 · Yes"}},{v:1,l:{es:"1 · No",en:"1 · No"}}]},
      {id:28,type:"enum",range:[0,2],def:2, name:{es:"Anti-atraso",en:"Anti-tailgating"}, desc:{es:"Estrategia anti cola.",en:"Anti-queuing."},
        options:[{v:0,l:{es:"0 · Rebote",en:"0 · Rebound"}},{v:2,l:{es:"2 · Cierre forzado",en:"2 · Force close"}}]},
      {id:29,type:"number",range:[1,9],def:3, name:{es:"Umbral desvío",en:"Deviation Threshold"}, desc:{es:"Tolerancia límite.",en:"Limit tolerance."}},
      {id:30,type:"enum",range:[0,1],def:1, name:{es:"IR libre",en:"Free-Pass IR"}, desc:{es:"Antipellizco libre.",en:"Free anti-pinch."},
        options:[{v:0,l:{es:"0 · No",en:"0 · No"}},{v:1,l:{es:"1 · Sí",en:"1 · Yes"}}]},
      {id:31,type:"enum",range:[0,1],def:0, name:{es:"Memoria libre",en:"Free-Pass Memory"}, desc:{es:"Acumula pasos.",en:"Accumulate steps."},
        options:[{v:0,l:{es:"0 · No",en:"0 · No"}},{v:1,l:{es:"1 · Sí",en:"1 · Yes"}}]},
      {id:32,type:"number",range:[0,9],def:0, name:{es:"Comp. Maestro",en:"Master Compensation"}, desc:{es:"Deslizamiento maestro.",en:"Master slip."}},
      {id:33,type:"number",range:[0,9],def:0, name:{es:"Comp. Esclavo",en:"Slave Compensation"}, desc:{es:"Deslizamiento esclavo.",en:"Slave slip."}},
      {id:34,type:"enum",range:[0,1],def:0, name:{es:"Lógica IR",en:"IR Logic"}, desc:{es:"Validación grupos.",en:"Group validation."},
        options:[{v:0,l:{es:"0 · 4 grupos",en:"0 · 4 groups"}},{v:1,l:{es:"1 · 6 grupos",en:"1 · 6 groups"}}]},
      {id:35,type:"enum",range:[1,4],def:1, name:{es:"Panel Maestro",en:"Master Panel Light"}, desc:{es:"Colores frontal.",en:"Front colors."},
        options:[{v:1,l:{es:"1 · Estándar",en:"1 · Std"}},{v:3,l:{es:"3 · Verde fija",en:"3 · Constant Green"}}]},
      {id:36,type:"enum",range:[1,4],def:2, name:{es:"Panel Esclavo",en:"Slave Panel Light"}, desc:{es:"Colores frontal.",en:"Front colors."}, options:null}
    ];

    P[4].options = P[3].options;
    P[36].options = P[35].options;

    const getLang = () => localStorage.getItem('lang') || 'es';

    function createValueInput(item, cur, lang) {
      if (item.type === "enum") {
        const sel = document.createElement("select");
        sel.className = "val-input";
        item.options.forEach(opt => {
          const o = document.createElement("option");
          o.value = opt.v;
          o.textContent = opt.l[lang] || opt.v;
          if (String(opt.v) === String(cur)) o.selected = true;
          sel.appendChild(o);
        });
        return sel;
      } else {
        const inp = document.createElement("input");
        inp.type = "number";
        inp.className = "val-input";
        inp.min = item.range[0]; inp.max = item.range[1];
        inp.value = (cur !== null && cur !== undefined) ? cur : item.def;
        return inp;
      }
    }

    async function renderTable() {
      const langSel = getLang();
      const t = dict[langSel];
      
      document.getElementById('t-title').textContent = t.title;
      document.getElementById('t-conf-sub').textContent = t.sub;
      document.getElementById('th-id').textContent = t.thId;
      document.getElementById('th-fn').textContent = t.thFn;
      document.getElementById('th-desc').textContent = t.thDesc;
      document.getElementById('th-range').textContent = t.thRange;
      document.getElementById('th-val').textContent = t.thVal;
      document.getElementById('th-act').textContent = t.thAct;
      document.getElementById('btnBackMenu').textContent = t.back;

      const tb = document.querySelector('#confTable tbody'); 
      
      try {
        const res = await fetch('/get_all_params');
        if (!res.ok) throw new Error("HTTP Error");
        const data = await res.json();
        const values = data.values;

        tb.innerHTML = '';
        
        P.forEach((m, index) => {
          const curValue = values[index];
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td>${m.id}</td>
            <td style="font-weight:700">${m.name[langSel]}</td>
            <td class="desc-col">${m.desc[langSel]}</td>
            <td style="font-size:11px; color:var(--muted)">${m.range[0]}-${m.range[1]} (def:${m.def})</td>
            <td id="cell_${m.id}"></td>
            <td><button onclick="saveParam(${m.id})">${t.btnSave}</button></td>
          `;
          
          const input = createValueInput(m, curValue, langSel);
          input.id = `p_${m.id}`;
          tr.querySelector(`#cell_${m.id}`).appendChild(input);
          tb.appendChild(tr);
        });

      } catch (e) {
        console.error(e);
        tb.innerHTML = `<tr><td colspan="6" style="color:red; text-align:center">${t.errConn}</td></tr>`;
      }
    }

    async function saveParam(id) {
      const lang = getLang();
      const t = dict[lang];
      const input = document.getElementById('p_' + id);
      const val = input.value;
      
      if (!confirm(t.confirm)) return;

      try {
        const res = await fetch('/params_save', {
          method: 'POST',
          headers: {'Content-Type': 'application/x-www-form-urlencoded'},
          body: `id=${id}&val=${val}`
        });
        const data = await res.json();
        if (data.status === 'ok') {
          alert(t.success);
        } else {
          alert('Error: ' + data.msg);
        }
      } catch (e) {
        alert(t.errConn);
      }
    }

    function setLang(lang) {
        localStorage.setItem('lang', lang);
        document.getElementById('btn-es').classList.toggle('active', lang === 'es');
        document.getElementById('btn-en').classList.toggle('active', lang === 'en');
        renderTable();
    }

    document.getElementById('btn-es').onclick = () => setLang('es');
    document.getElementById('btn-en').onclick = () => setLang('en');

    window.onload = () => {
        const currentLang = getLang();
        setLang(currentLang);
    };
  </script>
</body>
</html>)HTML";
//=============================================================

// ==================== HTML_STATUS ============================
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
      transition:all .15s ease;
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
      .site-header img{ height:44px; }
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
        <h2>{{TITLE}}</h2>
      </div>

      <div class="card-body">
        <div class="badge {{KIND}}">
          <span class="dot"></span>
          <span id="txt-kind">{{KIND}}</span>
        </div>

        <div class="msg">{{MESSAGE}}</div>

        <div class="actions">
          <a class="btn" href="{{BACK_URL}}">{{BACK_TEXT}}</a>
        </div>

        <div class="hint" id="txt-redirect">{{REDIRECT_HINT}}</div>
      </div>
    </div>
  </div>

  <script>
    // 1. Lógica de Redirección Automática
    (function(){
      var url = "{{AUTO_REDIRECT_URL}}";
      var ms  = parseInt("{{AUTO_REDIRECT_MS}}", 10);
      if (url && url !== "0" && !isNaN(ms) && ms > 0) {
        setTimeout(function(){ window.location.href = url; }, ms);
      }
    })();

    // 2. Lógica de Idioma para elementos no inyectados por el servidor
    const langMap = {
es: {
        SUCCESS_ID: "¡ÉXITO!",
        ERROR_ID: "ERROR",
        INFO_ID: "INFORMACIÓN",
        PROCESS_DONE_ID: "Operación completada correctamente.",
        BACK_ID: "Volver al menú",
        REDIRECT_MSG: "Redirigiendo automáticamente...",
        AUTH_ERROR_ID: "Contraseña incorrecta."
      },
      en: {
        SUCCESS_ID: "SUCCESS!",
        ERROR_ID: "ERROR",
        INFO_ID: "INFORMATION",
        PROCESS_DONE_ID: "Operation completed successfully.",
        BACK_ID: "Back to menu",
        REDIRECT_MSG: "Redirecting automatically...",
        AUTH_ERROR_ID: "Invalid password."
      }
    };

    window.onload = () => {
      const lang = localStorage.getItem('selectedLang') || 'es';
      document.documentElement.lang = lang;

      // Traducir el Badge (KIND) si coincide con los estándares
      const kindEl = document.getElementById('txt-kind');
      const currentKind = kindEl.innerText.toLowerCase();
      if(langMap[lang][currentKind]) {
        kindEl.innerText = langMap[lang][currentKind];
      }
    };
  </script>
</body>
</html>)HTML";
//============================================================

//======================= HTML UPLOAD ========================
static const char HTML_UPLOAD[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Actualizar Firmware ESP32</title>

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
      position: relative;
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .lang-select {
      position: absolute;
      right: 20px;
      display: flex;
      gap: 8px;
    }

    .lang-btn {
      background: rgba(255,255,255,0.2);
      border: 1px solid #fff;
      color: #fff;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }

    .lang-btn.active { background: #fff; color: var(--dark); }

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
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2 id="txt-header">Actualizar firmware</h2>
      </div>

      <div class="card-body">
        <p><span id="txt-curr-ver">Versión actual del firmware:</span> <span class="pill">VERSION_FIRMWARE</span></p>
        <p id="txt-select-file">Seleccione el archivo binario para actualizar el firmware.</p>

        <form method="POST" action="/upload_firmware" enctype="multipart/form-data">
          <input type="file" name="firmware" required />
          <div class="actions">
            <input type="submit" id="btn-submit" value="Actualizar firmware" />
            <button class="btn btn-danger" type="button" onclick="downloadFirmware()" id="btn-server">Descargar firmware desde servidor</button>
            <a class="btn" href="/menu" id="btn-back">Volver al menú</a>
          </div>
        </form>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Actualizar Firmware ESP32",
        header: "Actualizar firmware",
        currVer: "Versión actual del firmware:",
        selectFile: "Seleccione el archivo binario para actualizar el firmware.",
        btnSubmit: "Actualizar firmware",
        btnServer: "Descargar firmware desde servidor",
        btnBack: "Volver al menú",
        msgStart: "Descarga iniciada, el dispositivo se reiniciará en breve.",
        msgErr: "Error al iniciar descarga: ",
        msgConnErr: "Error de comunicación: "
      },
      en: {
        title: "Update ESP32 Firmware",
        header: "Update Firmware",
        currVer: "Current firmware version:",
        selectFile: "Select the binary file to update the firmware.",
        btnSubmit: "Update firmware",
        btnServer: "Download firmware from server",
        btnBack: "Back to menu",
        msgStart: "Download started, the device will reboot shortly.",
        msgErr: "Error starting download: ",
        msgConnErr: "Communication error: "
      }
    };

    let currentLang = 'es';

    function setLang(lang) {
      currentLang = lang;
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      
      document.title = t.title;
      document.getElementById('txt-header').innerText = t.header;
      document.getElementById('txt-curr-ver').innerText = t.currVer;
      document.getElementById('txt-select-file').innerText = t.selectFile;
      document.getElementById('btn-submit').value = t.btnSubmit;
      document.getElementById('btn-server').innerText = t.btnServer;
      document.getElementById('btn-back').innerText = t.btnBack;
      
      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
      document.documentElement.lang = lang;
    }

    function downloadFirmware() {
      const t = translations[currentLang];
      fetch('/download_firmware')
        .then(response => {
          if (response.ok) alert(t.msgStart);
          else response.text().then(text => alert(t.msgErr + text));
        })
        .catch(error => alert(t.msgConnErr + error));
    }

    window.onload = () => {
      const savedLang = localStorage.getItem('selectedLang') || 'es';
      setLang(savedLang);
    };
  </script>
</body>
</html>)HTML";
// ============================================================================

// ======================= HTML_REINICIAR ========================
static const char HTML_REINICIAR[] PROGMEM = R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title id="txt-title">Reiniciar dispositivo</title>

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
      position: relative;
    }

    .site-header img {
      height: 52px;
      max-width: 100%;
      object-fit: contain;
      filter: drop-shadow(0 2px 6px rgba(0, 0, 0, .25));
    }

    .lang-select {
      position: absolute;
      right: 20px;
      display: flex;
      gap: 8px;
    }

    .lang-btn {
      background: rgba(255,255,255,0.2);
      border: 1px solid #fff;
      color: #fff;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }

    .lang-btn.active { background: #fff; color: var(--dark); }

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
      .lang-select { position: relative; right: auto; margin-top: 10px; }
      .site-header { flex-direction: column; padding: 10px; }
    }
  </style>
</head>

<body>
  <header class="site-header">
    <img src="/qualicard.png" alt="Qualica-RD">
    <div class="lang-select">
      <button onclick="setLang('es')" class="lang-btn" id="btn-es">ES</button>
      <button onclick="setLang('en')" class="lang-btn" id="btn-en">EN</button>
    </div>
  </header>

  <div class="page">
    <div class="card">
      <div class="card-header">
        <h2 id="txt-header">Reiniciar dispositivo</h2>
      </div>
      <div class="card-body">
        <p id="txt-desc">Haz clic en el botón para reiniciar el equipo. Útil para aplicar cambios o resolver incidencias.</p>
        <div class="actions">
          <button class="reboot" type="button" onclick="confirmAndReboot()" id="btn-reboot">Reiniciar ahora</button>
          <a class="btn" href="/menu" id="btn-back">Volver al menú</a>
        </div>
      </div>
    </div>
  </div>

  <script>
    const translations = {
      es: {
        title: "Reiniciar dispositivo",
        header: "Reiniciar dispositivo",
        desc: "Haz clic en el botón para reiniciar el equipo. Útil para aplicar cambios o resolver incidencias.",
        reboot: "Reiniciar ahora",
        back: "Volver al menú",
        confMsg: "¿Estás seguro de que quieres reiniciar el dispositivo? Esto puede tardar unos segundos.",
        okMsg: "Reiniciando. Recarga la página en unos segundos.",
        errMsg: "Error al intentar reiniciar.",
        connErr: "Error de conexión al intentar reiniciar."
      },
      en: {
        title: "Reboot Device",
        header: "Reboot Device",
        desc: "Click the button to restart the equipment. Useful for applying changes or resolving issues.",
        reboot: "Reboot now",
        back: "Back to menu",
        confMsg: "Are you sure you want to reboot the device? This may take a few seconds.",
        okMsg: "Rebooting. Please refresh the page in a few seconds.",
        errMsg: "Error attempting to reboot.",
        connErr: "Connection error while attempting to reboot."
      }
    };

    let currentLang = 'es';

    function setLang(lang) {
      currentLang = lang;
      localStorage.setItem('selectedLang', lang);
      const t = translations[lang];
      
      document.title = t.title;
      document.getElementById('txt-header').innerText = t.header;
      document.getElementById('txt-desc').innerText = t.desc;
      document.getElementById('btn-reboot').innerText = t.reboot;
      document.getElementById('btn-back').innerText = t.back;
      
      document.getElementById('btn-es').classList.toggle('active', lang === 'es');
      document.getElementById('btn-en').classList.toggle('active', lang === 'en');
      document.documentElement.lang = lang;
    }

    function confirmAndReboot() {
      const t = translations[currentLang];
      if (confirm(t.confMsg)) {
        fetch('/reiniciar_dispositivo', { method: 'POST' })
          .then(r => r.ok ? alert(t.okMsg) : alert(t.errMsg))
          .catch(() => alert(t.connErr));
      }
    }

    // --- Lógica de Inactividad ---
    const INACTIVITY_TIMEOUT_MS = 300000;
    let timeoutId;

    function resetTimer() {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => window.location.href = "/logout", INACTIVITY_TIMEOUT_MS);
    }

    const events = ['mousemove', 'keypress', 'click', 'scroll', 'touchstart', 'keydown'];
    events.forEach(ev => document.addEventListener(ev, resetTimer, false));

    window.onload = () => {
      const savedLang = localStorage.getItem('selectedLang') || 'es';
      setLang(savedLang);
      resetTimer();
    };

    document.addEventListener("visibilitychange", () => {
      if (document.hidden) clearTimeout(timeoutId);
      else resetTimer();
    });
  </script>
</body>
</html>)HTML";
//==============================================================================================

// ============================================================================
// 2) Infra: escribir desde PROGMEM a LittleFS sin copiar a RAM
// ============================================================================

struct PageDef
{
  const char *path;
  const char *contentP; // PROGMEM
};

static size_t progmemStrLen(const char *p)
{
  size_t n = 0;
  while (pgm_read_byte(p + n) != 0)
    n++;
  return n;
}

static bool writeFileFromProgmem(fs::FS &fs, const char *path, const char *contentP)
{
  // Sobrescribe SIEMPRE
  if (fs.exists(path))
  {
    fs.remove(path);
  }

  File f = fs.open(path, FILE_WRITE);
  if (!f)
  {
    Serial.printf("[FS] ERROR open %s\n", path);
    return false;
  }

  const size_t len = progmemStrLen(contentP);

  // Escritura en chunks para ir rápido
  const size_t CHUNK = 512;
  uint8_t buf[CHUNK];

  size_t off = 0;
  while (off < len)
  {
    size_t n = len - off;
    if (n > CHUNK)
      n = CHUNK;

    for (size_t i = 0; i < n; i++)
    {
      buf[i] = (uint8_t)pgm_read_byte(contentP + off + i);
    }

    size_t w = f.write(buf, n);
    if (w != n)
    {
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
    {"/config.html", HTML_CONFIG},
    {"/actions.html", HTML_ACTIONS},
    {"/params.html", HTML_PARAMS},
    {"/upload_fs", HTML_FICHEROS}, // “Actualizar sistema de ficheros”
    {"/index.html", HTML_INDEX},       // si tú sirves "/" desde otro, cambia el nombre
    {"/logs.html", HTML_LOGS},
    {"/menu.html", HTML_MENU},
    {"/reiniciar.html", HTML_REINICIAR},
    {"/status.html", HTML_STATUS}, // plantilla para redirectStatus()
    {"/upload.html", HTML_UPLOAD},

};

// ============================================================================
// 4) Función pública: crea/sobrescribe todos los HTML en LittleFS
// ============================================================================

bool ensureWebPagesInLittleFS(bool formatOnFail)
{
  if (!LittleFS.begin(formatOnFail))
  {
    Serial.println("[FS] ERROR LittleFS.begin()");
    return false;
  }

  bool okAll = true;
  for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++)
  {
    const bool ok = writeFileFromProgmem(LittleFS, pages[i].path, pages[i].contentP);
    okAll = okAll && ok;
  }

  return okAll;
}
