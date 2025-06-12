// taskSQLITE.h — Versión simplificada y robusta
#ifndef TASK_SQLITE_H
#define TASK_SQLITE_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <sqlite3.h>
#include "dataStructures.h"
#include <esp_system.h>          // para esp_restart()

#define SD_CS             10
#define DB_DIR            "/sd/"

int contSDError =0;



// Sentencias de creación de tablas
static const char* SQL_CREATE_GPS_DATA =
  "CREATE TABLE IF NOT EXISTS gps_data ("
  " timestamp TEXT PRIMARY KEY,"
  " lat REAL, lon REAL, ele REAL, speed REAL,"
  " course REAL, sat INTEGER, hdop REAL,"
  " temp REAL, hum REAL,"
  " ax REAL, ay REAL, az REAL,"
  " gx REAL, gy REAL, gz REAL,"
  " mx REAL, my REAL, mz REAL,"
  " ref_ax REAL, ref_ay REAL, ref_az REAL"
  ");";

static const char* SQL_CREATE_RES_SUM =
  "CREATE TABLE IF NOT EXISTS resumen_ruta ("
  " id INTEGER PRIMARY KEY AUTOINCREMENT,"
  " timestamp_inicio TEXT, timestamp_fin TEXT,"
  " duracion_segundos INTEGER, distancia_km REAL,"
  " muestras_gps INTEGER, media_Hz REAL"
  ");";

// Variables internas
static sqlite3* db         = nullptr;

bool initializeDatabase();
bool writeBufferToSQLite(const GPSData* buffer, int count, const char* name);
bool appendBufferedData();
bool guardarResumenRuta();
void flushAndCloseDatabase();
void detenerModoGPS();

void flushAndCloseDatabase() {
  flushRequested = true;
  xTaskNotifyGive(TaskSQLITEHandle);  // Despierta la tarea taskSQLITE
}
void generateNewLogFileName() {
  snprintf(logFileName, 64,
    "%04d%02d%02d-%02d%02d%02d.db",
    gpsData.agno, gpsData.mes, gpsData.dia,
    gpsData.hora, gpsData.minuto, gpsData.segundo);
}

// Recupera tras detectar corrupción crea nueva DB
void recoverDatabase() {
  Serial.println("---Base de datos corrupta iniciando otra---");
  bdCorrupta=true;
  // Cerrar BD corrupta
  if (db) sqlite3_close_v2(db);
  // Rutas
  char oldPath[80], newPath[100];
  snprintf(oldPath, sizeof(oldPath), "%s%s", DB_DIR, logFileName);
  snprintf(newPath, sizeof(newPath), "%s%s-corrupta.db", DB_DIR, logFileName);
  SD.rename(oldPath, newPath);
  // Generar nuevo nombre de DB y reabrir
  generateNewLogFileName();
  dbReady = false;
  initializeDatabase();
}
// Inicializa SD y base de datos una sola vez
bool initializeDatabase() {
//  Montar SD
  if (!SD.begin(SD_CS, SPI, 1000000U, "/sd")) {
    sdReady = false;
    Serial.println("❌ Error montando SD");
    return false;
  }
  //  Prueba de escritura
  File f = SD.open("/.ping", FILE_WRITE);
  if (!f) {
    sdReady = false;
    Serial.println("❌ SD test write falló");
    return false;
  }
  f.close();
  SD.remove("/.ping");
  sdReady = true;
  Serial.println("✅ SD montada OK");

  // Esperar sincronización GPS para tener logFileName 
  
  while (!gpsSync) vTaskDelay(pdMS_TO_TICKS(500));

  
  char path[80];
  snprintf(path, sizeof(path), "%s%s", DB_DIR, logFileName);
  if (sqlite3_open(path, &db) != SQLITE_OK) return false;

  sqlite3_exec(db, "PRAGMA journal_mode=WAL;",    NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA synchronous=FULL;",   NULL, NULL, NULL);

  // Crear tablas si no existen
  sqlite3_exec(db, SQL_CREATE_GPS_DATA, NULL, NULL, NULL);
  sqlite3_exec(db, SQL_CREATE_RES_SUM,   NULL, NULL, NULL);
bdCorrupta=false;
  dbReady = true;
  return true;
}

// Escritura con prepared statements

bool writeBufferToSQLite(const GPSData* buffer, int count, const char* name) {
  if ( count <= 0) {
    Serial.printf("⚠️ Buffer %s vacío o BD no lista\n", name);
    return true; 
  }

  // Preparar statement
  const char* sql =
    "INSERT OR IGNORE INTO gps_data ("
    "timestamp,lat,lon,ele,speed,course,sat,hdop,temp,hum,"  
    "ax,ay,az,gx,gy,gz,mx,my,mz,ref_ax,ref_ay,ref_az)"
    " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("❌ prepare stmt buffer %s failed: %s\n", name, sqlite3_errmsg(db));
    recoverDatabase();
    return false;
  }

  // Contar cambios antes
  int antes = sqlite3_total_changes(db);
  sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
  Serial.printf("🔄 Escribiendo buffer %s: %d registros...\n", name, count);

  for (int i = 0; i < count; ++i) {
    const GPSData &d = buffer[i];
   
  char ts[32];
  snprintf(ts, sizeof(ts),
    "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
     d.agno, d.mes,   d.dia,
     d.hora, d.minuto,d.segundo,
     d.milisegundos    
  );

  sqlite3_bind_text(stmt, 1, ts, -1, SQLITE_TRANSIENT);
  
      sqlite3_bind_double(stmt,  2, d.lat);

    sqlite3_bind_double(stmt,  3, d.lon);
    sqlite3_bind_double(stmt,  4, d.ele);
    sqlite3_bind_double(stmt,  5, d.speed);
    sqlite3_bind_double(stmt,  6, d.course);
    sqlite3_bind_int(stmt,     7, d.satelites);
    sqlite3_bind_double(stmt,  8, d.hdop);
    sqlite3_bind_double(stmt,  9, d.temp);
    sqlite3_bind_double(stmt, 10, d.hum);
    sqlite3_bind_double(stmt, 11, d.ax);
    sqlite3_bind_double(stmt, 12, d.ay);
    sqlite3_bind_double(stmt, 13, d.az);
    sqlite3_bind_double(stmt, 14, d.gx);
    sqlite3_bind_double(stmt, 15, d.gy);
    sqlite3_bind_double(stmt, 16, d.gz);
    sqlite3_bind_double(stmt, 17, d.mx);
    sqlite3_bind_double(stmt, 18, d.my);
    sqlite3_bind_double(stmt, 19, d.mz);
    sqlite3_bind_double(stmt, 20, d.ref_ax);
    sqlite3_bind_double(stmt, 21, d.ref_ay);
    sqlite3_bind_double(stmt, 22, d.ref_az);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
      Serial.printf("⚠️ Error en step buffer %s idx %d: %s\n", name, i, sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
      recoverDatabase();
      return false;
    }
    sqlite3_reset(stmt);

    // Cede CPU cada 16 iteraciones para refrescar el watchdog
    if ((i & 0x0F) == 0) vTaskDelay(1);
  }

  sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
  sqlite3_finalize(stmt);

  int despues = sqlite3_total_changes(db);
  int insertados = despues - antes;
  Serial.printf("✅ Buffer %s: insertados %d de %d registros.\n", name, insertados, count);
  bufferReadyForSD=true;
  return true;
}



bool appendBufferedData() {
  if (!dbReady) {
    Serial.println("⚠️ BD no lista");
    return false;
  }

  bool ok = true;
  if (bufferIndexA > 0) {
    unsigned long ta = micros();


    if (!writeBufferToSQLite(gpsBufferA, bufferIndexA,"A")) {
      Serial.println("❌ Error A");
      ok = false;
    } else {
      bufferIndexA = 0;
    }
  }
  if (bufferIndexB > 0) {
        unsigned long t0 = micros();

    if (!writeBufferToSQLite(gpsBufferB, bufferIndexB, "B")) {
      Serial.println("❌ Error B");
      ok = false;
    } else {
            bufferIndexB = 0;

    

    }
  }
  return ok;
}


bool guardarResumenRuta() {
  if (db == nullptr) {
    Serial.println("⚠️ No se puede guardar el resumen: BD no abierta.");
    modoAlarma = true;
    return false;
  }

  char ts_fin[32];
  snprintf(ts_fin, sizeof(ts_fin),
           "%04d-%02d-%02dT%02d:%02d:%02dZ",
           relojAgno, relojMes, relojDia,
           relojHora, relojMinuto, relojSegundo); 

 
  unsigned long duracion = tiempoTotalMs / 1000;  // en s


  const char* sql =
    "INSERT INTO resumen_ruta "
    "(timestamp_inicio, timestamp_fin, duracion_segundos, "
      "distancia_km, muestras_gps, media_Hz) "
    "VALUES (?, ?, ?, ?, ?, ?);";

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    Serial.printf("❌ prepare resumen: %s\n", sqlite3_errmsg(db));
    modoAlarma = true;
    return false;
  }

  sqlite3_bind_text  (stmt, 1, timestampInicioRuta, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text  (stmt, 2, ts_fin,             -1, SQLITE_TRANSIENT);
  sqlite3_bind_int   (stmt, 3, (int)duracion);
  sqlite3_bind_double(stmt, 4, distanciaTotalKm);
  sqlite3_bind_int   (stmt, 5, (int)totalMuestrasGPS);
  sqlite3_bind_double(stmt, 6, hzMedia);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("❌ step resumen: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    modoAlarma = true;
    return false;
  }
  sqlite3_finalize(stmt);

  Serial.println("✅ Resumen de ruta guardado correctamente.");
  modoAlarma = false;
  return true;
}


void taskSQLITE(void* parameter) {
    const int MAX_INTENTOS = 20;
  int intentos = 0;
  // Reintentos de inicialización
  while (intentos < MAX_INTENTOS) {
    if (initializeDatabase()) {
      break;
    }
    intentos++;
    Serial.printf("⚠️ Init BD fallida (%d/%d). Reintentando en 2s…\n",
                  intentos, MAX_INTENTOS);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
  if (intentos >= MAX_INTENTOS) {
    Serial.printf("❌ %d intentos fallidos. Reiniciando...\n", MAX_INTENTOS);
    esp_restart();
  }

  // Bucle principal: espera notificaciones para escribir o flush

  for (;;) {
    // Espera notificación para append o flush
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (flushRequested) {
      flushStatus = FLUSH_PENDING;
       // guardamos buffers A y B
  bool okA = appendBufferedData(); // devuelve true si se insertaron bien
  bool okB = appendBufferedData(); // idem para buffer B
  bool okRes = guardarResumenRuta(); // dappevuelve true si insertó el resumen

  // cerramos BD
  sqlite3_close_v2(db);
  db = nullptr;
  dbReady = false;

  // actualizamos flushStatus según resultado
  if (okA && okB && okRes){
    flushStatus = FLUSH_SUCCESS;
  } 
  else{
     flushStatus = FLUSH_ERROR;
  }
  // limpieza y fin
  flushRequested = false;
} else {
    // solo volcados “en caliente” cuando el buffer se haya llenado:
    if (bufferIndexA >= BUFFER_SIZE) {
      bufferReadyForSD=false;
      writeBufferToSQLite(gpsBufferA, bufferIndexA, "A");
      bufferIndexA = 0;
    }
    if (bufferIndexB >= BUFFER_SIZE) {
            bufferReadyForSD=false;

      writeBufferToSQLite(gpsBufferB, bufferIndexB, "B");
      bufferIndexB = 0;
    }
       }
  }
}
}
#endif  
