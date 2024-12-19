#include "weather.h"
#include "weather_gui.h"
#include "weather_config.h"
#include "app/app_name.h"
#include "ESP32Time.h"
#include "sys/app_controller.h"
#include "network.h"
#include "ArduinoJson.h"
#include <esp32-hal-timer.h>
#include <map>

#include <WiFi.h>
#include "gui_lock.h"

bool isUdpInit = false;

struct WeatherAppRunData
{
    unsigned long preWeatherMillis; // 上一回更新天气时的毫秒数
    unsigned long preTimeMillis;    // 更新时间计数器
    long long preNetTimestamp;      // 上一次的网络时间戳
    long long errorNetTimestamp;    // 网络到显示过程中的时间误差
    long long preLocalTimestamp;    // 上一次的本地机器时间戳
    unsigned int coactusUpdateFlag; // 强制更新标志
    unsigned int updateState; // 表示当前是否处在更新状态

    BaseType_t xReturned_task_refresh; // 更新数据的异步任务
    TaskHandle_t xHandle_task_refresh; // 更新数据的异步任务

    Weather wea;     // 保存天气状况
    TimeStr screenTime; // 屏幕显示的时间
};

static WeatherAppRunData *run_data = NULL;

static void updateWeather(void)
{
    if (WL_CONNECTED != WiFi.status())
        return;

    HTTPClient http;
    http.setTimeout(1000);
    char api[128] = {0};


    // 暂时用tianqi_appid，当CITY_CODE
    // 使用WEATHER_API_KEY当WEATHER_API_KEY
    snprintf(api, 128, WEATHER_NOW_API_UPDATE,
             cfg_data.WEATHER_API_KEY.c_str(),
             cfg_data.CITY_CODE.c_str());
    log_i("API = %s", api);
    http.begin(api);

    int httpCode = http.GET();
    String payload = http.getString();
    DynamicJsonDocument doc(768);
    deserializeJson(doc, payload);
    if (httpCode > 0)
    {
        log_i("%s", payload.c_str());
        if (doc.containsKey("lives"))
        {
            JsonObject weather_live = doc["lives"][0];
            // 获取城市区域中文
            strcpy(run_data->wea.cityname, weather_live["city"].as<String>().c_str());
            // 温度
            run_data->wea.temperature = weather_live["temperature"].as<int>();
            // 湿度
            run_data->wea.humidity = weather_live["humidity"].as<int>();
            //天气情况
            run_data->wea.weather_code = weatherMap[weather_live["weather"].as<String>()];
            //log_i("wea.weather_code = %d", run_data->wea.weather_code);
            strcpy(run_data->wea.weather, weather_live["weather"].as<String>().c_str());
            //风速
            strcpy(run_data->wea.windDir, weather_live["winddirection"].as<String>().c_str());
            strcpy(run_data->wea.windpower , weather_live["windpower"].as<String>().c_str());
            log_i("wea.windpower  = %s", run_data->wea.windpower);

            //空气质量没有这个参数，只能用风速来粗略替换了
//            run_data->wea.airQulity = airQulityLevel(run_data->wea.windpower);

            log_i(" Get weather info OK");
        }
        else
        {
            // 返回值错误，记录
            log_i("[APP] Get weather error,info");
            String err_info = doc["info"];
            log_i("%s", err_info.c_str());
        }
    }
    else
    {
        log_e("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

static long long get_timestamp()
{
    // 使用本地的机器时钟
    run_data->preNetTimestamp = run_data->preNetTimestamp + (GET_SYS_MILLIS() - run_data->preLocalTimestamp);
    run_data->preLocalTimestamp = GET_SYS_MILLIS();
    return run_data->preNetTimestamp;
}

// 从ntp服务器同步时间戳
static bool ntp_sync()
{
    if (WL_CONNECTED != WiFi.status()){
        log_w("wifi not connect");
        return false;
    }

    if(!isUdpInit)
    {
        // 初始化 NTP 客户端
        timeClient.begin();
        log_i("timeClient.begin()");
    }

    // 更新 NTP 时间
    if (!timeClient.update()){
        log_w("timeClient.update(): failed"); // timeClient默认时间在1970年，而太早的时间会导致ESP32Time库变得超级慢。
        return false;
    }

    unsigned long long epochTime;
    log_i("timeClient.update(): success.");
    epochTime = timeClient.getEpochTime(); // 获取当前时间戳

    // 将时间戳转换为本地时间（加上时区偏移）
    unsigned long long localTime = epochTime + gmtOffset_sec;
    log_i("local timestamp (UTC+8): %llu", localTime);

    // 将本地时间转换为日期时间格式
    time_t rawTime = localTime;
    struct tm* timeinfo = localtime(&rawTime);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    log_i("local time: %s", buffer);

    run_data->preNetTimestamp = epochTime*1000 + run_data->errorNetTimestamp;   //秒的时间戳变ms的
    run_data->preLocalTimestamp = GET_SYS_MILLIS();
    log_i("run_data->preNetTimestamp=%lld", run_data->preNetTimestamp);
    log_i("run_data->preLocalTimestamp=%lld", run_data->preLocalTimestamp);

    return true;
}

// 更新屏幕显示时间的数据
// A time less than 2017 year makes ESP32Time.getTime() slow
// https://github.com/espressif/arduino-esp32/issues/8837
static void updateTimeRTC(long long timestamp)
{
    struct TimeStr t;
    ESP32Time g_rtc;
    log_d("updateTimeRTC() start");
    g_rtc.setTime(timestamp / 1000);
    t.month = g_rtc.getMonth() + 1;
    t.day = g_rtc.getDay();
    t.hour = g_rtc.getHour(true);
    t.minute = g_rtc.getMinute();
    t.second = g_rtc.getSecond();
    t.weekday = g_rtc.getDayofWeek();
    // log_i("time : %d-%d-%d\n",t.hour, t.minute, t.second);
    log_d("updateTimeRTC() return");
    run_data->screenTime = t;
}

// 自动刷新缓冲区和屏幕
static void refreshScreen(void *parameter)
{
    while (1)
    {
        LVGL_OPERATE_LOCK(render_state(run_data->updateState);)
        LVGL_OPERATE_LOCK(render_weather(run_data->wea);)
        LVGL_OPERATE_LOCK(render_time(run_data->screenTime);)
        LVGL_OPERATE_LOCK(render_man();)
        LVGL_OPERATE_LOCK(lv_task_handler();)
        vTaskDelay(1000.0 / 60 / portTICK_PERIOD_MS);
    }
}

/***************************  app接口  ****************************/
static int weather_init(AppController *sys)
{
    tft->setSwapBytes(true);
    weather_gui_init();
    display_weather_init();
    // 获取配置信息
    read_config(&cfg_data);

    // 初始化运行时参数
    run_data = (WeatherAppRunData *)calloc(1, sizeof(WeatherAppRunData));
    memset((char *)&run_data->wea, 0, sizeof(Weather));
    run_data->preNetTimestamp = 1577808000000; // 上一次的网络时间戳 初始化为2020-01-01 00:00:00 todo 把它放到持久数据里，这样即使退出天气app也不会重置时间
    run_data->errorNetTimestamp = 2;
    run_data->preLocalTimestamp = GET_SYS_MILLIS(); // 上一次的本地机器时间戳
    run_data->preWeatherMillis = 0;
    run_data->preTimeMillis = 0;
    // 强制更新
    run_data->coactusUpdateFlag = 0x01;
    run_data->updateState = WEATHER_UPDATING; // 表示正在更新

    // 后台异步渲染
    run_data->xReturned_task_refresh = xTaskCreate(
            refreshScreen,                 /*任务函数*/
            "refreshScreen",                  /*带任务名称的字符串*/
            8 * 1024,                     /*堆栈大小，单位为字节*/
            NULL,                         /*作为任务输入传递的参数*/
            1,                            /*任务的优先级*/
            &run_data->xHandle_task_refresh); /*任务句柄*/

    return 0;
}

static void weather_process(AppController *sys,
                            const ImuAction *act_info)
{
    if (DOWN_MORE == act_info->active)
    {
        sys->app_exit();
        return;
    }
    //else if (GO_FORWORD == act_info->active)
    else if (UP == act_info->active) // todo 改成需要最小时间间隔启动，更新时有提示
    {
        // 间接强制更新
        run_data->coactusUpdateFlag = 0x01;
        run_data->updateState = WEATHER_UPDATING;
        delay(1000); // 以防间接强制更新后，生产很多请求 使显示卡顿
    }

    if (0x01 == run_data->coactusUpdateFlag || doDelayMillisTime(cfg_data.weatherUpdataInterval, &run_data->preWeatherMillis, false))
    {
        sys->send_to(WEATHER_APP_NAME, WIFI_SYS_NAME,
                     APP_MESSAGE_WIFI_STA, (void *)UPDATE_NOW, NULL);
    }

    if (0x01 == run_data->coactusUpdateFlag || doDelayMillisTime(cfg_data.timeUpdataInterval, &run_data->preTimeMillis, false))
    {
        // 尝试同步网络上的时钟
        sys->send_to(WEATHER_APP_NAME, WIFI_SYS_NAME,
                     APP_MESSAGE_WIFI_STA, (void *)UPDATE_NTP, NULL);
    }
    else if (GET_SYS_MILLIS() - run_data->preLocalTimestamp > 200) //间隔应该是1000ms的因数
    {
        updateTimeRTC(get_timestamp()); // 刷新run_data->screenTime。分离计算screenTime和刷新屏幕的过程，减少无效计算。
    }
    run_data->coactusUpdateFlag = 0x00; // 取消强制更新标志

    delay(100);
}

static int weather_exit_callback(void *param)
{
    weather_gui_del();

    // 查杀异步任务
    if (pdPASS == run_data->xReturned_task_refresh)
    {
        vTaskDelete(run_data->xHandle_task_refresh);
    }

    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
}

static void weather_message_handle(const char *from, const char *to,
                                   APP_MESSAGE_TYPE type, void *message,
                                   void *ext_info)
{
    switch (type)
    {
    case APP_MESSAGE_WIFI_STA:
    {
        log_i("----->weather_event_notification");
        int event_id = (int)message;
        switch (event_id)
        {
        case UPDATE_NOW:
        {
            log_i("weather update.");
            updateWeather();
            run_data->updateState = WEATHER_UPDATED;
        };
        break;
        case UPDATE_NTP:
        {
            log_i("ntp update.");
            ntp_sync(); // nowapi时间API
            run_data->updateState = WEATHER_UPDATED;
        };
        break;
        default:
            break;
        }
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
        char *param_key = (char *)message;
        if (!strcmp(param_key, "tianqi_url"))
        {
            snprintf((char *)ext_info, 128, "%s", cfg_data.tianqi_url.c_str());
        }
        else if (!strcmp(param_key, "CITY_CODE"))
        {
            snprintf((char *)ext_info, 32, "%s", cfg_data.CITY_CODE.c_str());
        }
        else if (!strcmp(param_key, "WEATHER_API_KEY"))
        {
            snprintf((char *)ext_info, 33, "%s", cfg_data.WEATHER_API_KEY.c_str());
        }
        else if (!strcmp(param_key, "tianqi_addr"))
        {
            snprintf((char *)ext_info, 32, "%s", cfg_data.tianqi_addr.c_str());
        }
        else if (!strcmp(param_key, "weatherUpdataInterval"))
        {
            snprintf((char *)ext_info, 32, "%lu", cfg_data.weatherUpdataInterval);
        }
        else if (!strcmp(param_key, "timeUpdataInterval"))
        {
            snprintf((char *)ext_info, 32, "%lu", cfg_data.timeUpdataInterval);
        }
        else
        {
            snprintf((char *)ext_info, 32, "%s", "NULL");
        }
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
        char *param_key = (char *)message;
        char *param_val = (char *)ext_info;
        if (!strcmp(param_key, "tianqi_url"))
        {
            cfg_data.tianqi_url = param_val;
        }
        else if (!strcmp(param_key, "CITY_CODE"))
        {
            cfg_data.CITY_CODE = param_val;
        }
        else if (!strcmp(param_key, "WEATHER_API_KEY"))
        {
            cfg_data.WEATHER_API_KEY = param_val;
        }
        else if (!strcmp(param_key, "tianqi_addr"))
        {
            cfg_data.tianqi_addr = param_val;
        }
        else if (!strcmp(param_key, "weatherUpdataInterval"))
        {
            cfg_data.weatherUpdataInterval = atol(param_val);
        }
        else if (!strcmp(param_key, "timeUpdataInterval"))
        {
            cfg_data.timeUpdataInterval = atol(param_val);
        }
    }
    break;
    case APP_MESSAGE_READ_CFG:
    {
        read_config(&cfg_data);
    }
    break;
    case APP_MESSAGE_WRITE_CFG:
    {
        write_config(&cfg_data);
    }
    break;
    default:
        break;
    }
}

APP_OBJ weather_app = {WEATHER_APP_NAME, &app_weather, "",
                       weather_init, weather_process,
                       weather_exit_callback, weather_message_handle};
