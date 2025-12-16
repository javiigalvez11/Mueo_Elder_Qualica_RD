#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

// Contenido de la página HTML incrustado en PROGMEM
const char FICHEROS_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html>

<head>
  <title>Actualizar sistema de ficheros</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta charset="utf-8" />
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f4;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      color: #333;
    }

    .container {
      background-color: #ffffff;
      padding: 30px;
      border-radius: 8px;
      box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
      text-align: center;
      max-width: 400px;
      width: 90%;
      border: 1px solid #e0e0e0;
    }

    h2 {
      color: #333;
      margin-bottom: 20px;
      font-size: 2em;
      font-weight: bold;
    }

    p {
      color: #555;
      margin-bottom: 25px;
      font-size: 1.1em;
    }

    input[type="file"] {
      width: 100%;
      padding: 10px;
      margin-bottom: 20px;
      font-size: 1em;
      box-sizing: border-box;
      border-radius: 4px;
      border: 1px solid #ddd;
    }

    input[type="submit"],
    button {
      background-color: #6c757d;
      color: white;
      border: none;
      border-radius: 5px;
      padding: 12px 20px;
      font-size: 1.1em;
      cursor: pointer;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
      transition: background-color 0.2s ease, transform 0.1s ease;
      width: 100%;
      margin-top: 10px;
    }

    input[type="submit"]:hover,
    button:hover {
      background-color: #5a6268;
      transform: translateY(-1px);
    }
  </style>
</head>

<body>
  <div class="container">
    <h2>Actualizar sistema de ficheros</h2>
    <p>
      Sube archivos individuales para el sistema de ficheros. Solo se permiten
      extensiones <code>.html</code> y <code>.json</code>. Si el fichero existe
      se sobrescribirá, si no, se creará.
    </p>
    <!-- Aquí apuntas explícitamente al endpoint con puerto 8080 -->
    <form method="POST" action="http://192.168.4.1:8080/upload_fs" enctype="multipart/form-data">
      <input type="file" name="file" accept=".html,.json" required />
      <input type="submit" value="Subir fichero" />
    </form>
    <button onclick="window.location.href='/menu';">Volver al menú</button>
  </div>
</body>

</html>)HTML";

/// Crea el fichero en LittleFS si no existe.
/// Lo deja guardado "para siempre" en la flash.
void ensureUpdateFsPage() {
  // OJO: si ya montas LittleFS antes, puedes quitar este begin()
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] Error al montar LittleFS");
    return;
  }

  const char *path = "/ficheros.html";  // Nombre del fichero en LittleFS

  if (LittleFS.exists(path)) {
    Serial.println("[FS] Página ficheros.html ya existe, no se modifica");
    return;
  }

  File f = LittleFS.open(path, FILE_WRITE);
  if (!f) {
    Serial.println("[FS] Error al crear /ficheros.html");
    return;
  }

  f.print(FICHEROS_HTML);
  f.close();

  Serial.println("[FS] Página /ficheros.html creada correctamente");
}
