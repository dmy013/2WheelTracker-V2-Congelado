#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
// Stack sizes por tarea
extern uint32_t stackSizeTaskGPS;
extern uint32_t stackSizeTaskSQLITE;
extern uint32_t stackSizeTaskDisplay;



// 📌 Declarar los TaskHandles para cada tarea
extern TaskHandle_t TaskSDHandle;
extern TaskHandle_t TaskGPSHandle;

extern TaskHandle_t TaskDisplayHandle;
extern TaskHandle_t TaskSQLITEHandle;
extern TaskHandle_t TaskWebHandle;


// En dataStructures.h:
typedef struct {
  float mediaMs;
  float hz;
}
GPSRate;

extern GPSRate gpsRate;  // Declaración global













 // ← referencia acelerómetro global
 extern float cal_ax;  
extern float cal_ay;
extern float cal_az;
// 📌 Estructura para compartir datos del GPS
struct GPSData {
 float temp,hum;//BME
  float ax,ay,az;//acelerometro
  float mx,my,mz;//magnetometro
  float gx,gy,gz;//giroscopio
  float ref_ax,ref_ay,ref_az;//referencia acelerometro
  int dia, mes, agno, hora, minuto, segundo, milisegundos;
  float  ele, speed, course, hdop;
  double lat,lon;
  int satelites;
};
extern GPSData gpsData;
// Estados de resultado del flush

enum FlushStatus {
  FLUSH_IDLE,     // sin operación pendiente
  FLUSH_PENDING,  // flush en curso
  FLUSH_SUCCESS,  // flush completado OK
  FLUSH_ERROR     // flush terminó con error
};

// Variable compartida (volatile porque puede cambiar en otra tarea)
extern volatile FlushStatus flushStatus;

// 📌 Declarar el buffer y el índice de escritura

#define BUFFER_SIZE 75 // Cada buffer almacena x datos
//gestion de buffers
extern GPSData gpsBufferA[BUFFER_SIZE];
extern GPSData gpsBufferB[BUFFER_SIZE];
extern String direccionIP_AP ;

extern int bufferIndexA;
extern int bufferIndexB;
extern int sats;
extern float hdops;
extern float hzReciente;
 extern float lastTemp;
 extern float lastHum;
extern float segundos;
extern float rollDegree;
extern float pitchDegree;


extern bool writingBufferA;  // Indica cuál buffer está en uso
extern bool bufferReadyForSD;
extern bool sdReady;  // Indica si la SD está lista para escribir datos
extern bool dbReady;
extern bool touchReady;
extern bool dbBusy;      //  Indica si SQLite está ocupada guardando datos
extern bool espRestart;  //indica si se va a reiniciar el ESP-32
extern bool modoAlarma; // indica una situacion de alarma
extern bool dhtReady;
extern bool icmReady;
extern bool success;
extern bool dbInitialized;    //  Indica si la base de datos se abrió correctamente
extern bool bufferFull;       //  Indica si los buffers estan llenos
extern bool lastSavedBuffer;  //  `false = B`, `true = A`
extern bool gpsPaused ;
extern bool flushRequested;
extern bool estaEnMovimiento ;
extern bool fixValido;
extern bool bdCorrupta;


extern unsigned long ultimaMuestra ;
extern unsigned long tiempoActivoMs;
extern unsigned long totalMuestrasGPS;
extern double distanciaTotalKm;
extern double distanciaKm;
extern double velocidadKmh;
extern float hzMedia;
extern bool dbGuardado ;

extern float percDRAM ;



//variable para la tabla de resumen_data

extern char timestampInicioRuta[32];     // guardado en el primer fix GPS válido
extern char timestampFinRuta[32];        // generado al cerrar

extern unsigned long tiempoTotalMs;
extern unsigned long tiempoMovimientoMs;
extern unsigned long tiempoParadoMs;





//  Definir variables globales
extern char logFileName[32];  // Nombre del archivo por defecto
extern GPSData gpsBuffer[BUFFER_SIZE];
extern long lastMillis;
extern bool  lostConection,  gpsSync;
extern int relojHora, relojMinuto, relojSegundo, relojMilisegundos, satelite;
extern int relojDia, relojMes, relojAgno;
//  Crear colas para comunicación entre Tasks
extern QueueHandle_t gpsQueue;  // Para datos del GPS
extern QueueHandle_t calQueue;  // Para datos de la calibhracion


// Variables globales compartidas
extern bool sdError;    // Para detectar si la SD tiene fallos
extern int desconexion;
extern long lastUpdate;
extern uint8_t screenRotation;  // Cambia este valor según la orientacion
extern int x, y;

extern const char* wifi_ssid;
extern const char* wifi_pass;

#endif
