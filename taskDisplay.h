#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "dataStructures.h"  
#include "taskGPS.h"
#include "taskSQLITE.h"
#include "taskWeb.h"
void flushAndCloseDatabase();

#define TFT_CS   18
#define TFT_DC    9
#define TFT_RST  17
#define TOUCH_CS  4
#define TOUCH_IRQ 255

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

/* táctil → coordenadas pantalla (rot 1) */
#define TS_MINX 260
#define TS_MAXX 3700
#define TS_MINY 260
#define TS_MAXY 3700

#define UIFONT FreeSansBold9pt7b
const GFXfont* currentFont = &UIFONT;

const float BOTON_AREA_Y = 0.85f;   // 15 % inferior
const float SIDEBAR_X    = 0.70f;   // 30 % derecha
int bottomYpx, sidebarXpx;



// referencia global 
extern float cal_ax, cal_ay, cal_az;

//botones
struct Boton { float xP, yP, wP, hP; const char* txt; };
Boton bStart={0.05,0.25,0.90,0.20,"Iniciar GPS"};
Boton bSD   ={0.05,0.55,0.90,0.20,"Datos en SD"};
Boton bAnt  ={0.00,BOTON_AREA_Y,0.30,0.15,"<--"};
Boton bSig  ={0.70,BOTON_AREA_Y,0.30,0.15,"-->"};
Boton bVista={0.35,BOTON_AREA_Y,0.30,0.15,"Vista"};
Boton bToggle={SIDEBAR_X,0.28,0.30,0.15,"Pausa"};
Boton bCalib ={SIDEBAR_X,0.56,0.30,0.15,"Calib"};
Boton bFinal ={SIDEBAR_X,0.00,0.30,0.15,"Final"};
Boton bCalOK ={0.10,BOTON_AREA_Y,0.35,0.15,"Calibrar"};
Boton bVolver={0.55,BOTON_AREA_Y,0.35,0.15,"Volver"};
Boton bYes   ={0.10,0.55,0.35,0.15,"SI"};
Boton bNo    ={0.55,0.55,0.35,0.15,"No"};
Boton bResOK = {0.35, BOTON_AREA_Y, 0.30, 0.15, "Volver"};


enum Estado { GRABANDO, PAUSADO };
enum Vista  { MENU, V_INFO, V_STATUS, CALIB, SDVIEW, CONFIRM, RESULT};
Estado estado = PAUSADO;
Vista  vistaActual  = MENU, vistaPrev = V_INFO;
const uint8_t MAX_INFO = 3, MAX_STATUS = 3;
uint8_t pagInfo = 0, pagStat = 0;

const int C_R = 60, B_R = 6;
int cx, cy, calibCX, calibCY;
float refRoll = 0, refPitch = 0;


void drawBtn(const Boton& b, uint16_t col = ILI9341_BLUE) {
  int x = tft.width()*b.xP, y = tft.height()*b.yP;
  int w = tft.width()*b.wP, h = tft.height()*b.hP;
  tft.fillRect(x,y,w,h,col);
  tft.setFont(currentFont); tft.setTextColor(ILI9341_WHITE);
  int16_t x1,y1; uint16_t w1,h1;
  tft.getTextBounds(b.txt,0,0,&x1,&y1,&w1,&h1);
  tft.setCursor(x+(w-w1)/2, y+(h+h1)/2-2); tft.print(b.txt);
}
bool hit(const Boton& b,int tx,int ty){
  int x=tft.width()*b.xP,y=tft.height()*b.yP;
  int w=tft.width()*b.wP,h=tft.height()*b.hP;
  return tx>=x&&tx<=x+w&&ty>=y&&ty<=y+h;
}
void clearSide(){ tft.fillRect(sidebarXpx,0,tft.width()-sidebarXpx,bottomYpx,ILI9341_WHITE); }
void clearData(){ tft.fillRect(0, 0,sidebarXpx, bottomYpx, ILI9341_WHITE);}


void drawBubble(int ccx,int ccy,float roll,float pitch){
  tft.fillRect(ccx-C_R,ccy-C_R,C_R*2,C_R*2,ILI9341_WHITE);
  tft.drawCircle(ccx,ccy,C_R,ILI9341_BLACK);
  int bx=ccx-(roll /(0.35f))*(C_R-B_R);
  int by=ccy+(pitch/(0.35f))*(C_R-B_R);
  tft.fillCircle(bx,by,B_R,ILI9341_RED);
}


void iniciarModoGPS() {
  // GPS
  if (TaskGPSHandle == NULL) {
    xTaskCreatePinnedToCore(taskGPS, "TaskGPS", stackSizeTaskGPS  ,  NULL, 4, &TaskGPSHandle, 1);
    Serial.println("✅ taskGPS creada");
  } else {
    vTaskResume(TaskGPSHandle);
    Serial.println("▶️ taskGPS reanudada");
  }


  // SQLITE
  if (TaskSQLITEHandle == NULL) {
    xTaskCreatePinnedToCore(taskSQLITE,"TaskSQLITE" ,stackSizeTaskSQLITE, NULL, 3, &TaskSQLITEHandle, 0);
    Serial.println("✅ taskSQLITE creada");
  } else {
    vTaskResume(TaskSQLITEHandle);
    Serial.println("▶️ taskSQLITE reanudada");
  }


}
void detenerModoGPS() {
    gpsData = {};
  if (TaskGPSHandle != NULL) {
    TaskGPSHandle=NULL;
    vTaskDelete(TaskGPSHandle);
  }
  if (TaskSQLITEHandle != NULL){
    TaskSQLITEHandle=NULL;
    vTaskDelete(TaskSQLITEHandle);
  } 

}

void IniciarWeb() {
  // 🚫 Apagar pantalla y touch temporalmente
 
  if (TaskSQLITEHandle != NULL) vTaskSuspend(TaskSQLITEHandle);

  vTaskDelay(pdMS_TO_TICKS(1000));  // ⏳ pequeña espera para liberar SPI

  if (TaskWebHandle == NULL) {
    Serial.println("🟢 Iniciando WebServer...");
  xTaskCreatePinnedToCore(taskWebServer, "taskWeb", 10192, NULL, 1, &TaskWebHandle, 1);// task para crear servidor
  } else {
    Serial.println("ℹ️ taskWeb ya estaba creada");
  }
}

void detenerWeb() {

  if (TaskWebHandle != NULL) {
    WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
    vTaskDelete(TaskWebHandle);
    TaskWebHandle = NULL;
    Serial.println("🛑 taskWeb eliminada");
  }
}
void showResult() {
  tft.fillScreen(ILI9341_WHITE);
  tft.setFont(currentFont);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(20, 80);

  switch (flushStatus) {
    case FLUSH_PENDING:
      tft.print("Guardando ruta...");
      break;
    case FLUSH_SUCCESS:
      tft.print(" Ruta guardada OK");
      break;
    case FLUSH_ERROR:
      tft.print("Error al guardar ruta");
      break;
    default:
      tft.print("Estado desconocido");
  }

  // Mostrar botón para volver al menú (solo si no está pending)
  if (flushStatus != FLUSH_PENDING) {
    drawBtn(bResOK, ILI9341_BLUE);
  }
}

void showMenu(){ tft.fillScreen(ILI9341_WHITE);
tft.setFont(currentFont); tft.setTextColor(ILI9341_DARKCYAN);
tft.setCursor(30, 40);tft.setTextSize(2);
tft.print("2Wheel Tracker");
drawBtn(bStart,ILI9341_GREEN); drawBtn(bSD,ILI9341_BLUE);
tft.setTextSize(1); }

void drawInfoPage(uint8_t p){
      tft.setTextColor(ILI9341_DARKGREEN);
                tft.setCursor(10,20); tft.printf("INFO %d\n",p+1);


  switch(p){

    case 0: 

     tft.setCursor(10,40);
  tft.printf("Hora: %02d:%02d:%02d.%03d", gpsData.hora, gpsData.minuto, gpsData.segundo, gpsData.milisegundos);
     tft.setCursor(10,60);
  tft.printf("Fecha: %04d/%02d/%02d", gpsData.agno, gpsData.mes, gpsData.dia);

  tft.setCursor(10, 80);
  tft.printf("Lt:%.6f", gpsData.lat);  
   tft.setCursor(10, 100);
  tft.printf("Ln:%.6f", gpsData.lon);
   tft.setCursor(10, 120);
  tft.printf("Sat: %d / ", sats); tft.printf("HDOP: %.3f",hdops);
  tft.setCursor(10, 140);
  tft.printf("Temp: %.1f C", gpsData.temp);
    tft.setCursor(10, 160);

  tft.printf("Hum: %.1f %", gpsData.hum);
 

    
      break;
    case 1:
     tft.setCursor(10,40);
  tft.printf("Vel= %.2f", gpsData.speed);
   tft.setCursor(10,60);
  tft.printf("ALtura X= %.2f", gpsData.ele);
   tft.setCursor(10,80);
  tft.printf("Course X= %.2f", gpsData.course);
   tft.setCursor(10,100);
  tft.printf("Dist total = %.2f Km", distanciaTotalKm);
   tft.setCursor(10,120);
  tft.printf("T total = %.2f",tiempoTotalMs / 1000.0);
   tft.setCursor(10,140);

    tft.printf("en Mov: %s", estaEnMovimiento ? "SI" : "NO");


 
           break;
    case 2:
         tft.setCursor(10,40);
  tft.printf("aX= %.2f",gpsData.ax);
     tft.setCursor(10,60);
  tft.printf("ay= %.2f",gpsData.ay);

  tft.setCursor(10, 80);
  tft.printf("az= %.2f",gpsData.az);
       tft.setCursor(10,100);
       tft.printf("Velocidad gps : %.2fK/h",gps.speed.kmph());
       tft.setCursor(10,120);
       tft.printf("Orientacion : %.2f°",gps.cardinal(gps.course.deg()));
     
              break;
  }
}
void drawStatusPage(uint8_t p){
      tft.setTextColor(ILI9341_PURPLE);
        tft.setCursor(10,20); tft.printf("STATUS %d\n",p+1);


  switch(p){
    case 0:
  tft.setCursor(10, 40);
  tft.printf("SD: %s", sdReady ? "OK" : "FALLO");
  tft.setCursor(10, 60);
  tft.printf("BD: %s", dbReady ? "OK" : "FALLO");
  tft.setCursor(10, 80);
  tft.printf("ICM: %s", icmReady ? "OK" : "FALLO");
    tft.setCursor(10, 100);
  tft.printf("Bd Corrupta: %s", bdCorrupta ? "SI" : "NO");
      tft.setCursor(10,120);
  tft.printf("Ram libre= %.2f %", percDRAM); 
       break;
    case 1: 
 
  tft.setCursor(10, 40);
  tft.printf("Buffer listo: %s", bufferReadyForSD ? "SI" : "NO");

  tft.setCursor(10, 60);
  tft.printf("Buffer A: %d / %d", bufferIndexA, BUFFER_SIZE);
  tft.setCursor(10, 80);
  tft.printf("Buffer B: %d / %d", bufferIndexB, BUFFER_SIZE);
 tft.setCursor(10, 100);
  tft.printf("FIX: %s / ", fixValido ? "SI" : "No"); 
   tft.setCursor(10, 120);

  tft.printf("Desc: %d",desconexion);
     if (gpsPaused) {
      tft.setCursor(10, 140);
      tft.printf("Hz Rec: --.- Hz");
      tft.setCursor(10, 160);
      tft.printf("Hz Med: --.- Hz");
    } else {        
        tft.setCursor(10, 140);
      tft.printf("Hz Rec: %.2f Hz", hzReciente);
      tft.setCursor(10, 160);
      tft.printf("Hz Med: %.2f Hz", hzMedia);
     }

      break;
    case 2:
        tft.setCursor(10, 40);
    tft.printf("ref_ax: %.3f ",gpsData.ref_ax);
         tft.setCursor(10, 60);
    tft.printf("ref_ay: %.3f ",gpsData.ref_ay);
         tft.setCursor(10, 80);
    tft.printf("ref_az: %.3f ",gpsData.ref_az);
  
         break;
  }
}
void showInfo(){
  tft.fillScreen(ILI9341_WHITE);
  tft.setFont(currentFont); tft.setTextColor(ILI9341_BLACK);
  drawInfoPage(pagInfo);
  drawBtn(bAnt); drawBtn(bSig); drawBtn(bVista);
  clearSide();
  if(estado==GRABANDO){ bToggle.txt="Pausa"; drawBtn(bToggle,ILI9341_YELLOW);}
  else{ bToggle.txt="Play"; drawBtn(bToggle,ILI9341_GREEN); drawBtn(bCalib,ILI9341_CYAN); drawBtn(bFinal,ILI9341_RED);}
}


void showStatus(){
  tft.fillScreen(ILI9341_WHITE);
  tft.setFont(currentFont); tft.setTextColor(ILI9341_BLACK);
  drawStatusPage(pagStat);
  drawBtn(bAnt); drawBtn(bSig); drawBtn(bVista);
  clearSide();
  if(estado==GRABANDO){ bToggle.txt="Pausa"; drawBtn(bToggle,ILI9341_YELLOW);}
  else{ bToggle.txt="Play"; drawBtn(bToggle,ILI9341_GREEN); drawBtn(bCalib,ILI9341_CYAN); drawBtn(bFinal,ILI9341_RED);}
}

void showSD(){ 
  tft.fillScreen(ILI9341_WHITE); 
  tft.setFont(currentFont);
   tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(10,40);
   tft.fillScreen(ILI9341_WHITE);
          tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(20, 60);
      tft.printf("Conectese a la Red Wifi");
          tft.setCursor(20, 80);
      tft.printf("Para ver los archivos");
       tft.setCursor(20, 100);

      tft.printf("wifi_ssid: %s", wifi_ssid);
             tft.setCursor(20, 120);
      tft.printf("wifi_pass: %s", wifi_pass);
   tft.setCursor(20, 140);
  tft.printf("IP: 192.168.4.1");
 
     drawBtn(bVolver,ILI9341_RED); 

  }

void showConfirm(){ tft.fillScreen(ILI9341_WHITE);
  tft.setFont(currentFont); tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(10,30); tft.print("¿Finalizar ruta?"); drawBtn(bYes,ILI9341_GREEN); drawBtn(bNo,ILI9341_RED); }

void showCalib(){
  tft.fillScreen(ILI9341_WHITE);
  tft.setFont(currentFont); tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(20,20); tft.println("CALIBRAR POSICION");
 drawBubble(calibCX,calibCY,0,0);
 tft.setCursor(20,40);
 tft.println("Manten el dispositivo estable");
 

  

  drawBtn(bCalOK,ILI9341_GREEN); drawBtn(bVolver,ILI9341_RED);
}

void dispatchTouch(int tx,int ty){
  switch(vistaActual){
      case MENU:
        if(hit(bStart,tx,ty)){ 
          vistaActual=V_INFO;
         gpsPaused=true;
          showInfo();
         iniciarModoGPS();
         }
        else if(hit(bSD,tx,ty)){ vistaActual=SDVIEW; 
        showSD();
        IniciarWeb();
        }
        break;
      case SDVIEW:
        if(hit(bVolver,tx,ty)){ vistaActual=MENU;
        showMenu();
        detenerWeb();
        }
        break;
    case CALIB:
  if (hit(bVolver, tx, ty)){         
      vistaActual = V_INFO;
      showInfo();
  }
  else if (hit(bCalOK, tx, ty) && icmReady){   
     float sumX=0, sumY=0, sumZ=0;
  const int N = 20;
  for(int i=0; i<N; ++i) {
    sumX += gpsData.ax;
    sumY += gpsData.ay;
    sumZ += gpsData.az;
    tft.fillRect(190,165, 120, 20, ILI9341_WHITE);
          tft.setCursor(190,180);  

  tft.setTextColor(ILI9341_BLACK);

    tft.printf("Cal: %d/%d",i+1,N);
        vTaskDelay(pdMS_TO_TICKS(16));  

  }
  float ax = sumX/N, ay = sumY/N, az = sumZ/N;
cal_ax = ax;
cal_ay = ay;
cal_az = az;

    tft.fillRect(190,165, 120, 20, ILI9341_WHITE);
     tft.setTextColor(ILI9341_GREEN);
           tft.setCursor(190,180);  

    tft.printf("Cal OK");
         tft.setTextColor(ILI9341_BLACK);
 //Recalcula y dibuja la burbuja
  refRoll  = atan2(ay, az);
  refPitch = atan2(-ax, az);  
    drawBubble(calibCX, calibCY, 0, 0);


    


   
  }
  break;
      case V_INFO: case V_STATUS:
        if(hit(bVista,tx,ty)){ vistaActual=(vistaActual==V_INFO)?V_STATUS:V_INFO; vistaActual==V_INFO?showInfo():showStatus();}
        else if(hit(bToggle,tx,ty)){
           estado=(estado==GRABANDO)?PAUSADO:GRABANDO;
          gpsPaused = (estado == PAUSADO);  

            vistaActual==V_INFO?showInfo():showStatus();
           }
        else if(hit(bAnt,tx,ty)){
          if(vistaActual==V_INFO&&pagInfo>0) --pagInfo;
          if(vistaActual==V_STATUS&&pagStat>0) --pagStat;
          vistaActual==V_INFO?showInfo():showStatus();
        }else if(hit(bSig,tx,ty)){
          if(vistaActual==V_INFO&&pagInfo<MAX_INFO-1) ++pagInfo;
          if(vistaActual==V_STATUS&&pagStat<MAX_STATUS-1) ++pagStat;
          vistaActual==V_INFO?showInfo():showStatus();
        }else if(estado==PAUSADO && hit(bCalib,tx,ty)){ 
          vistaActual=CALIB; showCalib();
        }
        else if(estado==PAUSADO && hit(bFinal,tx,ty)){
           vistaPrev=vistaActual; vistaActual=CONFIRM; 
           showConfirm();
           }
        break;
      case CONFIRM:
        if(hit(bYes,tx,ty)){
          flushAndCloseDatabase();
           vistaActual=RESULT; 
           showResult();
           }
        else if(hit(bNo,tx,ty)){
           vistaActual=vistaPrev; vistaActual==V_INFO?showInfo():showStatus();}
        break;
        case RESULT:
          if (flushStatus != FLUSH_PENDING && hit(bResOK, tx, ty)) {
    detenerModoGPS();
    vistaActual = MENU;
    showMenu();
    flushStatus = FLUSH_IDLE; 
  }
  break;
    }
}

void taskDisplay(void *){
  tft.begin(); tft.setRotation(screenRotation);
 
   vTaskDelay(pdMS_TO_TICKS(1150));

  while (!ts.begin()) {
    Serial.println("❌ Error: No se detecta touchscreen. Esperando conexión...");
    vTaskDelay(pdMS_TO_TICKS(50));
    touchReady = false;
  }

  touchReady = true;
  Serial.println("✅ Touchscreen iniciado correctamente.");

 
 
  ts.setRotation(screenRotation);
  bottomYpx=tft.height()*BOTON_AREA_Y;
  sidebarXpx=tft.width()*SIDEBAR_X;
          vTaskDelay(pdMS_TO_TICKS(500));

  calibCX=tft.width()/2-40; calibCY=bottomYpx/2+20;
          vTaskDelay(pdMS_TO_TICKS(500));

  GPSData displayData; 

  vistaActual=MENU;
   showMenu();

        // copia local de lo último en la cola
  uint32_t lastBub=0;

  for(;;){
 if(xQueuePeek(gpsQueue, &displayData, 0) == pdTRUE) {
    gpsData = displayData;

    }
    if(ts.touched()){
      TS_Point p=ts.getPoint();
      int tx=map(p.x,TS_MINX,TS_MAXX,0,tft.width());
      int ty=map(p.y,TS_MINY,TS_MAXY,0,tft.height());
      dispatchTouch(tx,ty);
    }

 switch (vistaActual) {

      case V_INFO:  
    clearData();  //limpia solo el área de datos (no la sidebar)

        drawInfoPage(pagInfo);
        break;
      case V_STATUS:
          clearData();  // limpia solo el área de datos (no la sidebar)

          drawStatusPage(pagStat);

        break;
        case RESULT:

        showResult();
        break;
        case CALIB :
         float _ax, _ay, _az;
 
    _ax = gpsData.ax;
    _ay = gpsData.ay;
    _az = gpsData.az;
  

  float rawRoll  = atan2(_ay, _az) - refRoll;
  float rawPitch = atan2(-_ax, _az) - refPitch;

      float roll=rawRoll, pitch=rawPitch;
      const float LIM=0.35f;
      float len=sqrtf(roll*roll+pitch*pitch);
      if(len>LIM){ roll*=LIM/len; pitch*=LIM/len; }

      drawBubble(calibCX,calibCY,roll,pitch);

      int txtx=calibCX+C_R+10;
      int clearW=tft.width()-txtx+2;
      tft.fillRect(200,60,120,100,ILI9341_WHITE);
      tft.setFont(currentFont); tft.setTextColor(ILI9341_BLACK);
       rollDegree = roll*57.2958f;
       pitchDegree = pitch*57.2958f;
      tft.setCursor(txtx,80);
       tft.printf("Roll : %+6.1f°",rollDegree);
       tft.setCursor(txtx,100);
       tft.printf("Pitch : %+6.1f°",pitchDegree);
      tft.setCursor(txtx,120);  
      tft.printf("ax: %+6.1f°",_ax);
      tft.setCursor(txtx,140);  
      tft.printf("ay: %+6.1f°",_ay);
      tft.setCursor(txtx,160);  
      tft.printf("az: %+6.1f°",_az);
break;
    }

    
    if (vistaActual == CALIB) {
  vTaskDelay(pdMS_TO_TICKS(50));  //  Refresco más rápido para la burbuja 
} else {
  vTaskDelay(pdMS_TO_TICKS(500)); //  Refresco estándar
}

}
}
#endif
