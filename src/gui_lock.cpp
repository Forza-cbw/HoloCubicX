#include "gui_lock.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// lvgl handle的锁
SemaphoreHandle_t lvgl_mutex = xSemaphoreCreateMutex();

void waitForAinm(void) {
    while (lv_anim_count_running() > 0)
        vTaskDelay(100 / portTICK_PERIOD_MS);
}