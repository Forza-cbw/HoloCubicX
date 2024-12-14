#ifndef COMMON_H
#define COMMON_H

#define AIO_VERSION "2.1.10.9"
// v2.1.10.9
// 修改时间接口为阿里NTP服务器，新老天气一起修改

// v2.1.10.8
// 修改陀螺仪MPU6050到QMI8658

//修改by：神秘藏宝室
// v2.1.10.7 精简LHLXW应用，换个名字和logo，更新为love，只简单修改留下动态心应用，其他的已屏蔽,

//v2.1.10.6
//修复高德API显示31位问题

//v2.1.10.5
//修改屏幕分享输出功率模式的log
//tftespi的启动DMA，减少投屏超时问题

//v2.1.10.4
//修正电池供电进入不了视频

//v2.1.10.3
//修改aida64为配网时候的显示

//v2.1.10.2
//修改天气图片匹配BUG

//v2.1.10.1
//增加番茄时钟，动态心，贪吃蛇等app，同步溜马小哥2.1.10

//v2.1.6.8
//修改新版天气的API为高德天气API，因为易客天气只免费3个月。而高德每天免费30万次访问数
//修改天气用到的字库ch_font20,用于适配庞大的城市名字库
//修改七天天气为4天，因为高德的预报只有4天

//v2.1.6.7
//天气v0的api被攻击无效，切换使用v1的

//v2.1.6.6
//修正纪念日的“天”的位置偏差

//v2.1.6.5
//默认使能media内的功耗模式

//v2.1.6.4
//把纪念日的字体增加3500个常用汉字

//v2.1.6.2
//修正Weather最高和最低温度显示

//v2.1.6.1
//修复Weather的API，原来的API已废弃
//修复Weater的第二页y轴坐标显示
//默认开启性能模式【第一次烧录，或者清空重新刷固件有效】


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

// TFT屏幕接口
// #define PEAK
#ifdef PEAK
#define LCD_BL_PIN 12
/* Battery */
#define CONFIG_BAT_DET_PIN 37
#define CONFIG_BAT_CHG_DET_PIN 38
/* Power */
#define CONFIG_POWER_EN_PIN 21
#define CONFIG_ENCODER_PUSH_PIN 27
#else
#define LCD_BL_PIN 46
#endif

#define LCD_BL_PWM_CHANNEL 0

// 优先级定义(数值越小优先级越低)
// 最高为 configMAX_PRIORITIES-1
#define TASK_RGB_PRIORITY 0  // RGB的任务优先级
#define TASK_LVGL_PRIORITY 2 // LVGL的页面优先级

// lvgl 操作的锁
extern SemaphoreHandle_t lvgl_mutex;
// LVGL操作的安全宏（避免脏数据）
#define AIO_LVGL_OPERATE_LOCK(CODE)                          \
    if (pdTRUE == xSemaphoreTake(lvgl_mutex, portMAX_DELAY)) \
    {                                                        \
        CODE;                                                \
        xSemaphoreGive(lvgl_mutex);                          \
    }

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
    uint8_t backLight;            // 屏幕亮度（1-100）
    uint8_t rotation;             // 屏幕旋转方向
    uint8_t auto_calibration_mpu; // 是否自动校准陀螺仪 0关闭自动校准 1打开自动校准
    uint8_t mpu_order;            // 操作方向
};

#define GFX 0

#if GFX
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS -1 // Not connected
#define TFT_DC 2
#define TFT_RST 4 // Connect reset to ensure display initialises
#include <Arduino_GFX_Library.h>
extern Arduino_HWSPI *bus;
extern Arduino_ST7789 *tft;

#else
#include <TFT_eSPI.h>
/*
TFT pins should be set in path/to/Arduino/libraries/TFT_eSPI/User_Setups/Setup24_ST7789.h
*/
extern TFT_eSPI *tft;
#endif

#endif