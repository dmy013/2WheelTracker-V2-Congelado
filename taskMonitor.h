#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

#include <Arduino.h>
#include "dataStructures.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

void printFreeRAM() {
  size_t freeDRAM = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t totalDRAM = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  percDRAM = (freeDRAM * 100.0) / totalDRAM;
/*
  size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  size_t totalPSRAM = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  float percPSRAM = (totalPSRAM > 0) ? (freePSRAM * 100.0) / totalPSRAM : 0.0;
*/
 // Serial.printf("💾 DRAM: %.2f%% libre (%u / %u bytes)\n", percDRAM, freeDRAM, totalDRAM);
 // Serial.printf("💾 PSRAM: %.2f%% libre (%u / %u bytes)\n", percPSRAM, freePSRAM, totalPSRAM);
}

void taskMonitor(void* parameter) {
  for (;;) {

 //  Serial.println("📈 Monitor de sistema:");

    printFreeRAM();
   //Serial.println("--------------------------");
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

#endif
