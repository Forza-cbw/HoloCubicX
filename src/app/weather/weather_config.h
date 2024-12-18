// 应该只被weather.cpp引用

#ifndef HOLOCUBICX_WEATHER_CONFIG_H
#define HOLOCUBICX_WEATHER_CONFIG_H

// 更新使用高德地图
#include <map>

#include "NTPClient.h"
#include "common.h"

#define WEATHER_NOW_API_UPDATE "http://restapi.amap.com/v3/weather/weatherInfo?key=%s&city=%s&extensions=base"
#define UPDATE_WEATHER 0x01       // 更新天气
#define UPDATE_TIME 0x04          // 更新时间

// NTP 服务器信息
const char* ntpServer = "ntp.aliyun.com"; // 阿里云NTP服务器
const long  gmtOffset_sec = 8 * 3600;     // 中国时区（UTC+8）
const int   daylightOffset_sec = 0;       // 无需夏令时偏移
// 使用 UDP 连接
WiFiUDP ntpUDP;
// 创建NTP客户端 60秒同步一次（这个间隔应该没用，因为timeUpdataInterval制定了手动刷新时间的间隔）
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000);

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
        cfg->weatherUpdataInterval = 60000; // 天气更新的时间间隔60000(60s)
        cfg->timeUpdataInterval = 60000;    // 日期时钟更新的时间间隔60000(60s)
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

static WT_Config cfg_data;


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

#endif //HOLOCUBICX_WEATHER_CONFIG_H
