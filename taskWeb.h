#ifndef TASK_WEB_SERVER_H
#define TASK_WEB_SERVER_H
#include "dataStructures.h"

#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SD.h"
#include "taskSQLITE.h"

WebServer server(80);
void taskWebServer(void *parameter);

void manejarDescarga() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Falta el parámetro 'file'");
    return;
  }

  String filename = "/" + server.arg("file");
  File file = SD.open(filename);
  if (!file || file.isDirectory()) {
    server.send(404, "text/plain", "Archivo no encontrado");
    return;
  }

  server.sendHeader("Content-Type", "application/octet-stream");
  server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
  server.sendHeader("Connection", "close");
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void manejarBorrado() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Falta el parámetro 'file'");
    return;
  }

  String filename = "/" + server.arg("file");
  if (SD.exists(filename)) {
    SD.remove(filename);
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(404, "text/plain", "Archivo no encontrado");
  }
}

void listarArchivos() {
  File root = SD.open("/");

  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  float porcentaje = ((float)usedBytes / totalBytes) * 100.0;
  float totalMB = totalBytes / (1024.0 * 1024.0);
  float usedMB = usedBytes / (1024.0 * 1024.0);

  String pagina = R"rawliteral(
    <!DOCTYPE html>
    <html lang='es'>
    <head>
      <meta charset='utf-8'>
      <title>Archivos en la SD</title>
      <style>
        body { font-family: sans-serif; background: #f0f0f0; padding: 20px; }
        h2 { color: #333; display: flex; justify-content: space-between; align-items: center; }
        .refresh-btn {
          background-color: #ffc107;
          color: black;
          border: none;
          padding: 6px 12px;
          border-radius: 5px;
          cursor: pointer;
        }
        .refresh-btn:hover {
          background-color: #e0a800;
        }
        input[type='text'] {
          width: 100%;
          padding: 10px;
          margin-bottom: 15px;
          border: 1px solid #ccc;
          border-radius: 5px;
        }
        .progress-container {
          margin-bottom: 20px;
          background-color: #ddd;
          border-radius: 5px;
          overflow: hidden;
        }
        .progress-bar {
          height: 20px;
          background-color: #4CAF50;
          width: PERCENT%;
        }
        table {
          width: 100%;
          border-collapse: collapse;
          background: #fff;
          box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        th, td {
          padding: 12px;
          border-bottom: 1px solid #ddd;
          text-align: left;
        }
        th { background-color: #4CAF50; color: white; }
        tr:hover { background-color: #f5f5f5; }
        a.download-btn, a.delete-btn {
          padding: 6px 12px;
          text-decoration: none;
          border-radius: 5px;
          color: white;
        }
        a.download-btn { background-color: #2196F3; }
        a.download-btn:hover { background-color: #1976D2; }
        a.delete-btn { background-color: #f44336; }
        a.delete-btn:hover { background-color: #d32f2f; }
        .info-msg {
          font-size: 0.9em;
          color: #666;
          margin-bottom: 10px;
        }
      </style>
    </head>
    <body>
      <h2>
        Archivos en la SD
        <button class='refresh-btn' onclick='window.location.reload()'>🔁 Recargar</button>
      </h2>
      <p class='info-msg'>📥 Al descargar un archivo, la página puede quedar bloqueada temporalmente hasta que finalice la transferencia.</p>
      <p>Usado: USED MB / TOTAL MB (PERCENT%)</p>
      <div class='progress-container'><div class='progress-bar'></div></div>

      <input type='text' id='searchInput' placeholder='🔍 Buscar por nombre de archivo...' onkeyup='filtrarTabla()'>
      <table id='fileTable'>
        <thead>
          <tr><th>Nombre</th><th>Tamaño</th><th>Acciones</th></tr>
        </thead>
        <tbody>
  )rawliteral";

  pagina.replace("PERCENT", String(porcentaje, 1) );
  pagina.replace("USED", String(usedMB, 2));
  pagina.replace("TOTAL", String(totalMB, 2));

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    if (!entry.isDirectory()) {
      String nombre = entry.name();
      size_t size = entry.size();
      float tamKB = size / 1024.0;
      String tamanoStr = (tamKB > 1024) ? String(tamKB / 1024.0, 2) + " MB" : String(tamKB, 1) + " KB";

      pagina += "<tr><td>" + nombre + "</td>";
      pagina += "<td>" + tamanoStr + "</td>";
      pagina += "<td><a class='download-btn' href='/download?file=" + nombre + "' target='_blank'>Descargar</a> ";
      pagina += "<a class='delete-btn' href='#' onclick='confirmarBorrado(\"" + nombre + "\")'>Eliminar</a></td></tr>";
    }

    entry.close();
  }

  pagina += R"rawliteral(
        </tbody>
      </table>

      <script>
        function filtrarTabla() {
          const input = document.getElementById("searchInput").value.toLowerCase();
          const filas = document.querySelectorAll("#fileTable tbody tr");
          filas.forEach(fila => {
            const nombre = fila.cells[0].textContent.toLowerCase();
            fila.style.display = nombre.includes(input) ? "" : "none";
          });
        }

        function confirmarBorrado(filename) {
          if (confirm("¿Seguro que deseas eliminar " + filename + "?")) {
            window.location.href = "/delete?file=" + filename;
          }
        }
      </script>

    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", pagina);
}

void iniciarWiFiAP() {
  WiFi.softAP(wifi_ssid, wifi_pass);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("📶 Punto de acceso creado. IP: ");
  Serial.println(IP);
}


void taskWebServer(void *parameter) {
  iniciarWiFiAP();

 if (SD.begin(SD_CS, SPI, 1000000U, "/sd")) {
    File testRetry = SD.open("/.ping", FILE_WRITE);
    if (testRetry) {
      testRetry.close();
      SD.remove("/.ping");
      Serial.println("✅ SD funcional.");
          sdReady=true;
    }
 }
else{
Serial.println("❌Error al leer la tarjeta SD❌");
    vTaskDelay(pdMS_TO_TICKS(2000));
ESP.restart();
 }

  server.on("/", listarArchivos);
  server.on("/download", manejarDescarga);
  server.on("/delete", manejarBorrado);
  server.begin();
 direccionIP_AP = WiFi.softAPIP().toString();

  Serial.printf("🌐 Servidor web iniciado en http://%s" , direccionIP_AP);



  for (;;) {
    server.handleClient();
    Serial.println("server ok ✅");
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

#endif
