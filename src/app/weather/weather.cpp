#include "weather.h"
#include "weather_gui.h"
#include "ESP32Time.h"
#include "sys/app_controller.h"
#include "network.h"
#include "common.h"
#include "ArduinoJson.h"
#include <esp32-hal-timer.h>
#include <map>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define WEATHER_APP_NAME "Weather"
// 更新使用高德地图
#define WEATHER_NOW_API_UPDATE "http://restapi.amap.com/v3/weather/weatherInfo?key=%s&city=%s&extensions=base"
#define UPDATE_WEATHER 0x01       // 更新天气
#define UPDATE_TIME 0x04          // 更新时间

// NTP 服务器信息
const char* ntpServer = "ntp.aliyun.com"; // 阿里云NTP服务器
const long  gmtOffset_sec = 8 * 3600;     // 中国时区（UTC+8）
const int   daylightOffset_sec = 0;       // 无需夏令时偏移
// 使用 UDP 连接
WiFiUDP ntpUDP;
// 创建NTP客户端
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000); // 60秒同步一次
bool isUdpInit = false;

// 天气的持久化配置
#define WEATHER_CONFIG_PATH "/weather_219.cfg"
struct WT_Config
{
    String tianqi_url;                   // tianqiapi 的url
    String CITY_CODE;                 // tianqiapi 的 CITY_CODE
    String WEATHER_API_KEY;             // tianqiapi 的 WEATHER_API_KEY
    String tianqi_addr;                  // tianqiapi 的地址（填中文）
    unsigned long weatherUpdataInterval; // 天气更新的时间间隔(s)
    unsigned long timeUpdataInterval;    // 日期时钟更新的时间间隔(s)
};

static void write_config(WT_Config *cfg)
{
    char tmp[16];
    // 将配置数据保存在文件中（持久化）
    String w_data;
    w_data = w_data + cfg->tianqi_url + "\n";
    w_data = w_data + cfg->CITY_CODE + "\n";
    w_data = w_data + cfg->WEATHER_API_KEY + "\n";
    w_data = w_data + cfg->tianqi_addr + "\n";
    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->weatherUpdataInterval);
    w_data += tmp;
    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->timeUpdataInterval);
    w_data += tmp;
    g_flashCfg.writeFile(WEATHER_CONFIG_PATH, w_data.c_str());
}

static void read_config(WT_Config *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以APP名为开头 以".cfg"结尾，以免多个APP读取混乱
    char info[128] = {0};
    uint16_t size = g_flashCfg.readFile(WEATHER_CONFIG_PATH, (uint8_t *)info);
    info[size] = 0;
    if (size == 0)
    {
        // 默认值
        cfg->CITY_CODE = "340123";
        cfg->weatherUpdataInterval = 900000; // 天气更新的时间间隔900000(900s)
        cfg->timeUpdataInterval = 900000;    // 日期时钟更新的时间间隔900000(900s)
        write_config(cfg);
    }
    else
    {
        // 解析数据
        char *param[6] = {0};
        analyseParam(info, 6, param);
        cfg->tianqi_url = param[0];
        cfg->CITY_CODE = param[1];
        cfg->WEATHER_API_KEY = param[2];
        cfg->tianqi_addr = param[3];
        cfg->weatherUpdataInterval = atol(param[4]);
        cfg->timeUpdataInterval = atol(param[5]);
    }
}

struct WeatherAppRunData
{
    unsigned long preWeatherMillis; // 上一回更新天气时的毫秒数
    unsigned long preTimeMillis;    // 更新时间计数器
    long long preNetTimestamp;      // 上一次的网络时间戳
    long long errorNetTimestamp;    // 网络到显示过程中的时间误差
    long long preLocalTimestamp;    // 上一次的本地机器时间戳
    unsigned int coactusUpdateFlag; // 强制更新标志
    unsigned int update_type; // 更新类型的标志位

//    BaseType_t xReturned_task_task_update; // 更新数据的异步任务
//    TaskHandle_t xHandle_task_task_update; // 更新数据的异步任务

    Weather wea;     // 保存天气状况
    TimeStr screenTime; // 屏幕显示的时间
};

static WT_Config cfg_data;
static WeatherAppRunData *run_data = NULL;

enum wea_event_Id
{
    UPDATE_NOW,
    UPDATE_NTP,
};

std::map<String, int> weatherMap = {
    {"晴", 0},
    {"少云", 0},
    {"晴间多云", 3},
    {"多云", 3},
    {"阴", 1},
    {"有风", 3},
    {"平静", 3},
    {"微风", 3},
    {"和风", 3},
    {"清风", 3},
    {"强风/劲风", 3},
    {"疾风", 1},
    {"大风", 1},
    {"烈风", 1},
    {"风暴", 1},
    {"狂爆风", 1},
    {"飓风", 1},
    {"热带风暴", 1},
    {"霾", 5},
    {"中度霾", 5},
    {"重度霾", 5},
    {"严重霾", 5},
    {"阵雨", 2},
    {"雷阵雨", 7},
    {"雷阵雨并伴有冰雹", 4},
    {"小雨", 2},
    {"中雨", 2},
    {"大雨", 2},
    {"暴雨", 2},
    {"大暴雨", 2},
    {"特大暴雨", 2},
    {"强阵雨", 2},
    {"强雷阵雨", 7},
    {"极端降雨", 2},
    {"毛毛雨/细雨", 2},
    {"雨", 2},
    {"小雨-中雨", 2},
    {"中雨-大雨", 2},
    {"大雨-暴雨", 2},
    {"暴雨-大暴雨", 2},
    {"大暴雨-特大暴雨", 2},
    {"雨雪天气", 8},
    {"雨夹雪", 8},
    {"阵雨夹雪", 8},
    {"冻雨", 4},
    {"雪", 8},
    {"阵雪", 8},
    {"小雪", 8},
    {"中雪", 8},
    {"大雪", 8},
    {"暴雪", 8},
    {"小雪-中雪", 8},
    {"中雪-大雪", 8},
    {"大雪-暴雪", 8},
    {"浮尘", 6},
    {"扬沙", 6},
    {"沙尘暴", 6},
    {"强沙尘暴", 6},
    {"龙卷风", 6},
    {"雾", 5},
    {"浓雾", 5},
    {"强浓雾", 5},
    {"轻雾", 5},
    {"大雾", 5},
    {"特强浓雾", 5},
    {"热", 0},
    {"冷", 0},
    {"未知", 0}

};

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
    run_data->update_type = 0x00; // 表示什么也不需要更新

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
        delay(1000); // 以防间接强制更新后，生产很多请求 使显示卡顿
    }

    if (0x01 == run_data->coactusUpdateFlag || doDelayMillisTime(cfg_data.weatherUpdataInterval, &run_data->preWeatherMillis, false))
    {
        sys->send_to(WEATHER_APP_NAME, CTRL_NAME,
                     APP_MESSAGE_WIFI_CONN, (void *)UPDATE_NOW, NULL);
    }

    if (0x01 == run_data->coactusUpdateFlag || doDelayMillisTime(cfg_data.timeUpdataInterval, &run_data->preTimeMillis, false))
    {
        // 尝试同步网络上的时钟
        sys->send_to(WEATHER_APP_NAME, CTRL_NAME,
                     APP_MESSAGE_WIFI_CONN, (void *)UPDATE_NTP, NULL);
    }
    else if (GET_SYS_MILLIS() - run_data->preLocalTimestamp > 400)
    {
        updateTimeRTC(get_timestamp()); // 刷新run_data->screenTime。分离计算screenTime和刷新屏幕的过程，减少无效计算。
    }
    run_data->coactusUpdateFlag = 0x00; // 取消强制更新标志

    // 界面刷新
    display_weather(run_data->wea); // todo 分离成异步任务
    display_time(run_data->screenTime);
    display_space();
    delay(30);
}

static int weather_exit_callback(void *param)
{
    weather_gui_del();

    // 查杀异步任务
//    if (run_data->xReturned_task_task_update == pdPASS)
//    {
//        vTaskDelete(run_data->xHandle_task_task_update);
//    }

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
    case APP_MESSAGE_WIFI_CONN:
    {
        log_i("----->weather_event_notification");
        int event_id = (int)message;
        switch (event_id)
        {
        case UPDATE_NOW:
        {
            log_i("weather update.");
            run_data->update_type |= UPDATE_WEATHER;

            updateWeather();
        };
        break;
        case UPDATE_NTP:
        {
            log_i("ntp update.");
            run_data->update_type |= UPDATE_TIME;

            ntp_sync(); // nowapi时间API
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
