#include "HardwareSerial.h"

#ifndef TASK_GPS_H
#define TASK_GPS_H
#include <Arduino.h>
#include "dataStructures.h"
#include <TinyGPS++.h>  
#include "taskSQLITE.h" 
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "ICM_20948.h"
#include <math.h>


// Definir el pin donde está conectado el DHT11
#define DHTPIN 36  
#define DHTTYPE DHT11  // Tipo de sensor 
#define VELOCIDAD_MAX_ABSURDA 200.0  // km/h
#define UMBRAL_MOV 10.0

// Instancias de sensores
ICM_20948_I2C icm;
DHT dht(DHTPIN, DHTTYPE);
// Última vez que se tomó una medida válida
unsigned long ultimaMedidaOK = 0;
void taskGPS(void *param);
double calcularDistanciaKm(double lat1, double lon1, double lat2, double lon2);
double calcularVelocidadKmh(double dist, unsigned long tiempoMs);

// Intervalo máximo permitido sin medidas (en milisegundos)
const unsigned long pausaMaxMs = 10000;
unsigned long deltams;

//  Definir el pin donde está conectado el GPS
#define GPS_RX 42
#define GPS_TX 41
#define TOLERANCIA_GPS 5000
#define TIMEOUT_GPS 5000

// Filtro de media móvil para suavizar velocidad GPS
const int NUM_VELOCIDADES = 20;
double historialVelocidades[NUM_VELOCIDADES] = {0};
int indiceVelocidad = 0;
bool bufferVelocidadLleno = false;
double velocidad = 0.0;
double filtrarVelocidadMedia(double nuevaVelocidad);
double distancia=0.0;
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;
GPSData gpsData;

int syncTries = 0;

//  Variables  para llevar control global
unsigned long ultimoTiempo = 0;
static double ultimaLat = 0.0;
static double ultimaLon = 0.0;

void taskGPS(void *parameter);
void procesarGPS();
void leerSensoresEnGPS(GPSData &data);



void calcularHz() {
  static unsigned long ultimaMuestra = 0;
  static unsigned long tiempoActivoMs = 0;
  static unsigned long totalMuestras   = 0;

  static const int   VENTANA_SEGUNDOS     = 30;
  static const int   MAX_MUESTRAS_VENTANA = 300;  // para hasta 10 Hz
  static unsigned long marcasTiempo[MAX_MUESTRAS_VENTANA];
  static int        indiceVentana  = 0;
  static int        totalVentana   = 0;
  static bool       bufferLleno    = false;

  const unsigned long MAX_DT_VALIDO = 500;  // ms; umbral para salto (pausa)

  unsigned long ahora = millis();

  if (ultimaMuestra > 0) {
    unsigned long dt = ahora - ultimaMuestra;

    if (dt <= MAX_DT_VALIDO) {
      // 
      tiempoActivoMs += dt;
      totalMuestras++;

     
      marcasTiempo[indiceVentana] = ahora;
      indiceVentana = (indiceVentana + 1) % MAX_MUESTRAS_VENTANA;
      if (totalVentana < MAX_MUESTRAS_VENTANA) totalVentana++;
      else bufferLleno = true;
    }
    else {
      
    }
  }

  ultimaMuestra = ahora;

  
  if (tiempoActivoMs > 0) {
    hzMedia = (totalMuestras * 1000.0) / tiempoActivoMs;
  }


  int contador = 0;
  for (int i = 0; i < totalVentana; i++) {
    if ((ahora - marcasTiempo[i]) <= (VENTANA_SEGUNDOS * 1000)) {
      contador++;
    }
  }
  hzReciente = contador / (float)VENTANA_SEGUNDOS;
}



double filtrarVelocidadMedia(double nuevaVelocidad) {
  //  Si la nueva velocidad supera el umbral absurdo, la sustituimos
  if (nuevaVelocidad > VELOCIDAD_MAX_ABSURDA) {
    // Recuperamos el último valor válido del buffer
    int prevIdx = (indiceVelocidad - 1 + NUM_VELOCIDADES) % NUM_VELOCIDADES;
    nuevaVelocidad = historialVelocidades[prevIdx];
  }

  // Insertamos la velocidad (ya saneada) en el historial
  historialVelocidades[indiceVelocidad] = nuevaVelocidad;
  indiceVelocidad = (indiceVelocidad + 1) % NUM_VELOCIDADES;
  if (indiceVelocidad == 0) bufferVelocidadLleno = true;

  // Calculamos la media de las últimas N muestras
  int elementosValidos = bufferVelocidadLleno ? NUM_VELOCIDADES : indiceVelocidad;
  double suma = 0.0;
  for (int i = 0; i < elementosValidos; i++) {
    suma += historialVelocidades[i];
  }
  return suma / elementosValidos;
}

void procesarGPS() {
    double nuevaLat = gps.location.lat();
    double nuevaLon = gps.location.lng();

        // Calcular delta de tiempo entre dos medidas de procesar gps
    unsigned long ahora = millis();
    deltams = ahora - ultimoTiempo;

 // Evitar cálculo si aún no se ha registrado la primera posición válida
    if (ultimaLat == 0.0 && ultimaLon == 0.0) {
      ultimaLat = nuevaLat;
      ultimaLon = nuevaLon;
        ultimoTiempo = ahora;           // ← lo pones aquí

      Serial.println("🔄 Coordenadas iniciales registradas");
      return;
    }

// Si ha habido “pausa” , lo descartamos
if (deltams > pausaMaxMs) {
  deltams = 0;  
}

    ultimoTiempo = ahora;
    tiempoTotalMs += deltams;

   
if (deltams > 0){


    distancia = gps.distanceBetween(ultimaLat, ultimaLon, nuevaLat, nuevaLon);
distanciaKm=distancia/1000.0;
   velocidad =calcularVelocidadKmh(distanciaKm,deltams);
      velocidadKmh =filtrarVelocidadMedia(velocidad);

if (velocidadKmh>=UMBRAL_MOV){
 estaEnMovimiento = true;
    tiempoMovimientoMs += deltams;  
    } // acumula tiempo en movimiento}
else{
    estaEnMovimiento = false;
    tiempoParadoMs    += deltams;   // acumula tiempo parado
}
          distanciaTotalKm += distanciaKm;

}
else{
    distanciaKm = 0;
  velocidadKmh = 0;
      estaEnMovimiento = false;

}

    ultimaLat = nuevaLat;
    ultimaLon = nuevaLon;

        // Preparar datos GPS
    gpsData.lat = nuevaLat;
    gpsData.lon = nuevaLon;
    gpsData.ele = gps.altitude.meters();
    gpsData.speed = velocidadKmh;
    gpsData.course = gps.course.deg();
    gpsData.satelites = gps.satellites.value();
    gpsData.hdop= gps.hdop.hdop();

    leerSensoresEnGPS(gpsData);

    xQueueOverwrite(gpsQueue, &gpsData);

    // Guardar en buffer circular
    if (writingBufferA) {
      if (bufferIndexA < BUFFER_SIZE) {
        gpsBufferA[bufferIndexA++] = gpsData;
      } else {
        Serial.println("⚠️ Buffer A lleno → cambiando a B");
        writingBufferA = false;
        xTaskNotifyGive(TaskSQLITEHandle);
      }
    } else {
      if (bufferIndexB < BUFFER_SIZE) {
        gpsBufferB[bufferIndexB++] = gpsData;
      } else {
        Serial.println("⚠️ Buffer B lleno → cambiando a A");
        writingBufferA = true;
        xTaskNotifyGive(TaskSQLITEHandle);
      }
    }
    
    
   calcularHz();  
}


double calcularVelocidadKmh(double dist, unsigned long tiempoMs) {
  if (tiempoMs == 0) return 0.0;
  double tiempoHoras = tiempoMs / 3600000.0;
  return dist / tiempoHoras;
}



void leerSensoresEnGPS(GPSData &data) {
  icm.getAGMT();  // Actualiza los datos del sensor

  // DHT11
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp)) {
    data.temp = temp;
    lastTemp = temp;
  } else {
    data.temp = lastTemp;
  }

  if (!isnan(hum)) {
    data.hum = hum;
    lastHum = hum;
  } else {
    data.hum = lastHum;
  }

  // ICM20948 (IMU)
  data.ax = icm.accX();
  data.ay = icm.accY();
  data.az = icm.accZ();

  data.gx = icm.gyrX();
  data.gy = icm.gyrY();
  data.gz = icm.gyrZ();

  data.mx = icm.magX();
  data.my = icm.magY();
  data.mz = icm.magZ();

  // Referencia del acelerómetro (si está calibrada)
  data.ref_ax = cal_ax;
  data.ref_ay = cal_ay ;
data.ref_az = cal_az;
}

void checkGPSConnection() {
  while (gpsSerial.available()) {
  gps.encode(gpsSerial.read());
}
  sats = gps.satellites.value();
  hdops = gps.hdop.hdop();
  unsigned long ahora = millis();

  if (sats >= 4) {
    lastUpdate = ahora;
    desconexion = 0;
  } else if (ahora - lastUpdate > TIMEOUT_GPS && sats<4) {
    desconexion = (ahora - lastUpdate) / 1000;
    fixValido=false;
  }
}
bool esBisiesto(int agno) {
  return (agno % 4 == 0 && agno % 100 != 0) || (agno % 400 == 0);
}
// Función para ajustar día/mes/año correctamente
void ajustarDiaMesAgno() {
  const int diasPorMes[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int diasDelMes = diasPorMes[relojMes - 1];

  if (relojMes == 2 && esBisiesto(relojAgno)) {
    diasDelMes = 29;
  }

  if (relojDia > diasDelMes) {
    relojDia = 1;
    relojMes++;
    if (relojMes > 12) {
      relojMes = 1;
      relojAgno++;
    }
  }
}
void syncClockWithGPS() {

while (gpsSerial.available()) {
  gps.encode(gpsSerial.read());
  
}

  sats=gps.satellites.value();
  hdops=gps.hdop.hdop();

  if (!gpsPaused && gps.time.isValid() && gps.time.age() < 30000 && gps.date.isValid() && gps.satellites.value()>=4) {
    relojHora = gps.time.hour();
    relojMinuto = gps.time.minute();
    relojSegundo = gps.time.second();
    relojMilisegundos = millis() % 1000;  // Ajustar milisegundos al tiempo actual
    relojDia = gps.date.day();
    relojMes = gps.date.month();
    relojAgno = gps.date.year();
    lastMillis = millis();
    Serial.println(" ✅ GPS sincronizado");

    Serial.printf("⏳ Reloj interno calibrado con GPS: %04d/%02d/%02d %02d:%02d:%02d.%03d\n",
                  relojAgno, relojMes, relojDia, relojHora, relojMinuto, relojSegundo, relojMilisegundos);
    sprintf(logFileName, "/%04d%02d%02d-%02d%02d%02d.db",
            relojAgno, relojMes, relojDia, relojHora, relojMinuto, relojSegundo);

    sprintf(timestampInicioRuta, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            relojAgno, relojMes, relojDia, relojHora, relojMinuto, relojSegundo);

    Serial.printf("📁 Nombre de la base de datos: %s\n", logFileName);
    gpsSync = true;
    tiempoTotalMs      = 0;
    tiempoMovimientoMs = 0;
    tiempoParadoMs     = 0;



  } else {
    syncTries++;
    Serial.printf("⚠️ GPS no sincronizado. Esperando datos válidos... intento Nº: %d \n",syncTries);
       Serial.printf("📁 Numero de Satelites: %d\n", gps.satellites.value());

    vTaskDelay(pdMS_TO_TICKS(10));  
  }
}


void updateInternalClock() {
  unsigned long currentMillis = millis();
  unsigned long elapsedMillis = currentMillis - lastMillis;
  lastMillis = currentMillis;



  // Convertir milisegundos a segundos
  relojMilisegundos += elapsedMillis;
  while (relojMilisegundos >= 1000) {
    relojSegundo++;
    relojMilisegundos -= 1000;
  }

  //  Ajustar minutos
  while (relojSegundo >= 60) {
    relojMinuto++;
    relojSegundo -= 60;
  }

  //  Ajustar horas
  while (relojMinuto >= 60) {
    relojHora++;
    relojMinuto -= 60;
  }

  //  Ajustar días y meses
  if (relojHora >= 24) {
    relojHora = 0;
    relojDia++;
    ajustarDiaMesAgno();  //  Corrige cambios de mes y año
  }
  //  Guardar fecha en gpsData (para registros)
  gpsData.agno = relojAgno;
  gpsData.mes = relojMes;
  gpsData.dia = relojDia;
  gpsData.hora = relojHora;
  gpsData.minuto = relojMinuto;
  gpsData.segundo = relojSegundo;
  gpsData.milisegundos = relojMilisegundos;
  xQueueOverwrite(gpsQueue, &gpsData);  // Enviar SIEMPRE la hora al `taskDisplay`
}
void taskGPS(void *parameter) {
  gpsSerial.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  Wire.begin(38, 39); // Pines SDA, SCL

  dht.begin();

  Serial.println("✅ Sensor DHT11 listo");

 if (icm.begin(Wire, 0x69) == ICM_20948_Stat_Ok) {
    Serial.println("✅ ICM20948 detectado en 0x69");
    icmReady = true;
} else {
    Serial.println("❌ ICM20948 no detectado");
    icmReady = false;
}

   

  Serial.println("✅ Sensor ICM20948 listo");
  while (!gpsSync) {
    syncClockWithGPS();
    leerSensoresEnGPS(gpsData);
          xQueueOverwrite(gpsQueue, &gpsData);  // Actualizar solo la hora en pantalla

    vTaskDelay(pdMS_TO_TICKS(40));  //  Deja que otras tareas corran

  }

  for (;;) {
    //  Actualizar el reloj interno basado en `millis()`
    updateInternalClock();  // Actualizar el reloj interno
    checkGPSConnection();//comprueba la cantidad de satelites
    
//  Esperar activamente a que haya un fix GPS válido y actualizado
    if (gps.location.isUpdated() && !gpsPaused && gps.satellites.value() >= 4) {
       //  Si llegamos aquí, hay un fix GPS nuevo y válido
  //Serial.println(" ✅señal de gps adquirida✅");
      fixValido=true; 
      procesarGPS();
      totalMuestrasGPS++;
    vTaskDelay(pdMS_TO_TICKS(50));
    }
    else{
//Serial.println("Esperando señal de gps");
    leerSensoresEnGPS(gpsData);
      xQueueOverwrite(gpsQueue, &gpsData);  // Actualizar solo la hora en pantalla
    vTaskDelay(pdMS_TO_TICKS(50));
    }     
        vTaskDelay(pdMS_TO_TICKS(5));
  }
}


#endif
