#ifndef HOLOCUBICX_GUI_LOCK_H
#define HOLOCUBICX_GUI_LOCK_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// lvgl 操作的锁
extern SemaphoreHandle_t lvgl_mutex;
// LVGL操作的安全宏（避免脏数据），阻塞
#define LVGL_OPERATE_LOCK(CODE)                          \
    if (pdTRUE == xSemaphoreTake(lvgl_mutex, portMAX_DELAY)) \
    {                                                        \
        CODE;                                                \
        xSemaphoreGive(lvgl_mutex);                          \
    }
// LVGL操作的安全宏，非阻塞，失败就不执行代码
#define LVGL_OPERATE_TRY_LOCK(CODE)                        \
    if (pdTRUE == xSemaphoreTake(lvgl_mutex, 0))           \
    {                                                      \
        CODE;                                              \
        xSemaphoreGive(lvgl_mutex);                        \
    }

// 等待所有动画完成
// 不能放在LVGL_OPERATE_LOCK()中，因为后台程序无法刷新屏幕，lv_anim_count_running()不改变
void waitForAinm(void);

#ifdef __cplusplus
}
#endif

#endif //HOLOCUBICX_GUI_LOCK_H
