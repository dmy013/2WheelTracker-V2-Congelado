#include <Arduino.h>
#include "dataStructures.h"   
#include <SPI.h>
#include "taskDisplay.h"   
#include "taskGPS.h"    
#include "taskSQLITE.h"   
#include "taskWeb.h"
#include "taskMonitor.h"


// Tamaños de stack asignados a cada tarea (en bytes)
uint32_t stackSizeTaskGPS     = 4096;
uint32_t stackSizeTaskSQLITE  = 6500;
uint32_t stackSizeTaskDisplay = 7096;


// control de las Task
TaskHandle_t TaskGPSHandle = NULL;
TaskHandle_t TaskDisplayHandle = NULL;
TaskHandle_t TaskSQLITEHandle = NULL;
TaskHandle_t TaskWebHandle = NULL;



QueueHandle_t gpsQueue = NULL;  // Cola para el GPS


//  Definir buffers circulares del GPS 
GPSData gpsBufferA[BUFFER_SIZE];
GPSData gpsBufferB[BUFFER_SIZE];

GPSRate gpsRate = {0.0, 0.0}; 

int bufferIndexA = 0;
int bufferIndexB = 0;
int sats = 0;
float hdops=0.0;
float cal_ax = 0;
float cal_ay = 0; 
float cal_az = 0;
float rollDegree=0.0;
float pitchDegree=0.0;
bool bdCorrupta=false;




  float hzMedia = 0.0;
float hzReciente= 0.0;
 float lastTemp = -100.0;
 float lastHum = -100.0;
bool bufferReadyForSD = true;
bool writingBufferA = true;  //  Comienza usando `gpsBufferA`
bool sdReady = false;
bool dbReady = false;
bool flushRequested = false;
bool dhtReady= true;
bool icmReady = false;
bool lostConection = false;

bool gpsSync = false;
bool sdError = false;
bool success = true;  // Variable para verificar si hubo errores
bool dbInitialized = false; //  Indica si la base de datos se abrió correctamente
bool bufferFull = false; //  Indica si los buffers estan llenos
bool lastSavedBuffer = false;  //  `false = A`, `true = B`
bool dbBusy = false;  //  Indica si SQLite está ocupada guardando datos
bool espRestart = false; //indica si se va a reiniciar el ESP-32
bool touchReady = false;
bool gpsPaused = true;
bool estaEnMovimiento = false;
bool dbGuardado = false;
bool gpxGuardado = false;
bool fixValido = false;
double distanciaKm= 0.0;
 double velocidadKmh = 0.0;
unsigned long ultimaMuestra = 0;
unsigned long tiempoActivoMs = 0;
unsigned long totalMuestrasGPS = 0;
double distanciaTotalKm = 0;
float percDRAM = 0.0;
bool modoAlarma = false;



// variables para la tabla resumen_datos

char timestampInicioRuta[32] = "0000-00-00T00:00:00Z";
char timestampFinRuta[32] = "0000-00-00T00:00:00Z";
String direccionIP_AP ="";

unsigned long tiempoTotalMs = 0;
unsigned long tiempoMovimientoMs = 0;
unsigned long tiempoParadoMs = 0;

volatile FlushStatus flushStatus = FLUSH_IDLE;




float segundos = 0;
int relojHora = 0, relojMinuto = 0, relojSegundo = 0, relojMilisegundos = 0;
int relojDia = 1, relojMes = 1, relojAgno = 2024;
long lastUpdate = 0;
int desconexion = 0;
char logFileName[32] = "";  // Nombre del archivo
// Definición de `lastMillis`
long lastMillis = 0;
uint8_t  screenRotation= 1;  // Cambia este valor según necesites
int x=0, y=0;



//variables del Server
const char* wifi_ssid = "ESP32_Files";
const char* wifi_pass = "12345678";






void setup(){
    Serial.begin(115200);

          SPI.begin();  

  //  Crear colas de FreeRTOS
    gpsQueue = xQueueCreate(1, sizeof(GPSData));


 xTaskCreatePinnedToCore(taskMonitor,"TaskMonitor",3096,NULL,1,NULL, 1  );


 xTaskCreatePinnedToCore(taskDisplay, "TaskDisplay", stackSizeTaskDisplay, NULL, 3, &TaskDisplayHandle, 1); // Pantalla




}
void loop()
{
   
}