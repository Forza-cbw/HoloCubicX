#ifndef APP_WEATHER_GUI_H
#define APP_WEATHER_GUI_H

enum {
    WEATHER_STATUS_LATEST = 0,
    WEATHER_STATUS_EXPIRED,
    WEATHER_STATUS_UPDATING
};

struct Weather
{

    int maxTemp;      // 最高气温
    int minTemp;      // 最低气温

    int windLevel;
//    int airQulity;

    //高德天气API用到
    int weather_code; // 天气现象代码
    int temperature;  // 温度
    int humidity;     // 湿度
    char cityname[10]; // 城市名
    char windDir[20];
    char windpower[10]; //风力
    char weather[25];       //天气现象

    short daily_max[7];
    short daily_min[7];
};

struct TimeStr
{
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int weekday;
};

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

#define ANIEND                      \
    while (lv_anim_count_running()) \
        lv_task_handler(); //等待动画完成

    void weather_gui_init(void);
    void display_weather_init(void);
    void render_state(int timeStatus, int weatherStatus);
    void render_weather(struct Weather weaInfo);
    void render_time(struct TimeStr timeInfo);
    void weather_gui_release(void);
    void weather_gui_del(void);
    void render_man(void);
//    int airQulityLevel(char* q);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    extern const lv_img_dsc_t app_weather;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif