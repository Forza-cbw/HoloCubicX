#ifndef COMMON_H
#define COMMON_H

#define AIO_VERSION "2.1.10.9"
#define GET_SYS_MILLIS xTaskGetTickCount // 获取系统毫秒数
// #define GET_SYS_MILLIS millis            // 获取系统毫秒数

#include "Arduino.h"
#include "driver/rgb_led.h"
#include "driver/flash_fs.h"
#include "driver/sd_card.h"
#include "driver/display.h"
#include "driver/ambient.h"
#include "driver/imu.h"
#include "network.h"
#include "../../lib/TFT_eSPI/User_Setups/Setup24_ST7789.h"

#include "esp_log.h"

// SD_Card
#define SD_SCK 2
#define SD_MISO 40
#define SD_MOSI 1
#define SD_SS 38

//SD_MMC引脚定义
#define SDMC_CLK 2    //就是SD_SCK
#define SDMMC_CMD 38  //就是SD_MOSI
#define SDMMC_D0  1  //就是SD_MISO

// 陀螺仪
#define IMU_I2C_SDA 17
#define IMU_I2C_SCL 18

extern IMU mpu; // 原则上只提供给主程序调用
extern SdCard tf;
extern Pixel rgb;
// extern Config g_cfg;       // 全局配置文件
extern Network g_network;  // 网络连接
extern FlashFS g_flashCfg; // flash中的文件系统（替代原先的Preferences）
extern Display screen;     // 屏幕对象
extern Ambient ambLight;   // 光纤传感器对象

boolean doDelayMillisTime(unsigned long interval,
                          unsigned long *previousMillis,
                          boolean state);

// 光感 (与MPU6050一致)
#define AMB_I2C_SDA 17
#define AMB_I2C_SCL 18

// 屏幕尺寸
#define SCREEN_HOR_RES 240 // 水平
#define SCREEN_VER_RES 240 // 竖直

// 优先级定义(数值越小优先级越低)
// 最高为 configMAX_PRIORITIES-1
#define TASK_RGB_PRIORITY 0  // RGB的任务优先级
#define TASK_LVGL_PRIORITY 2 // LVGL的页面优先级

struct SysUtilConfig
{
    String ssid_0;
    String password_0;
    String ssid_1;
    String password_1;
    String ssid_2;
    String password_2;
    String auto_start_app;        // 开机自启的APP名字
    uint8_t power_mode;           // 功耗模式（0为节能模式 1为性能模式）
    uint8_t back_light;            // 屏幕亮度（1-100）
    uint8_t back_light2;            // 屏保屏幕亮度（1-100）
    uint32_t screensaver_interval;   // 屏保触发时间ms（无动作触发屏保）
    uint8_t rotation;             // 屏幕旋转方向
    uint8_t auto_calibration_mpu; // 是否自动校准陀螺仪 0关闭自动校准 1打开自动校准
    uint8_t mpu_order;            // 操作方向
};

#include <TFT_eSPI.h>
/*
TFT pins should be set in path/to/Arduino/libraries/TFT_eSPI/User_Setups/Setup24_ST7789.h
*/
extern TFT_eSPI *tft;

#endif