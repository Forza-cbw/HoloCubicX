#include "archer.h"
#include "archer_gui.h"
#include "app/app_name.h"
#include "sys/app_controller.h"
#include "common.h"
#include "network.h"
#include <PubSubClient.h>
#include "gui_lock.h"

#define DEFALUT_MQTT_IP "120.79.216.105"
#define DEFALUT_MQTT_IP_CLIMBL "climb.dynv6.net"
#define DEFALUT_MQTT_IP_CLIMBSNAIL "climbsnail.cn"
#define DEFALUT_MQTT_PORT 1883

// 持久化配置
#define ARCHER_CONFIG_PATH "/archer.cfg"

extern AppController *app_controller; // APP控制器

// 常驻数据，可以不随APP的生命周期而释放或删除
struct ArcherAppForeverData
{
    int role;                   // 0: heart, 1: beat
    char liz_mqtt_subtopic[32]; // "AIO-beat"
    char liz_mqtt_pubtopic[32]; // "AIO-heart"
    char client_id[32];         // 用户设置的ID
    char subtopic[32];          // 订阅的Topic
    char connect_client_id[64]; // 连接是使用的client_id default "AIO"+MAC+client_id
    char server_user[16];       // 服务的用户名
    char server_password[16];   // 服务的密码
    uint16_t port;              // mqtt服务器端口 1883
    IPAddress mqtt_server;
    WiFiClient espClient;      // 定义wifiClient实例
    PubSubClient *mqtt_client; //(mqtt_server, 1883, callback, espClient);
    static void callback(char *topic, byte *payload, unsigned int length);
    void mqtt_reconnect();
};

// 动态数据，APP的生命周期结束也需要释放它
struct HeartbeatAppRunData
{
    uint8_t send_cnt = 0;
    uint8_t recv_cnt = 0;
    unsigned long preUpdataMillis;     // 上一回更新的毫秒数
    unsigned long timeUpdataInterval;  // 更新时间计数器
    unsigned long heartContinueMillis; // 心跳的持续时间
    unsigned long lastHeartUpdataTime; // 上次心跳的更新时间
};

// 保存APP运行时的参数信息，理论上关闭APP时推荐在 xxx_exit_callback 中释放掉
static HeartbeatAppRunData *run_data = NULL;


void ArcherAppForeverData::callback(char *topic, byte *payload, unsigned int length)
{
    log_i("Message arrived [%s]", topic);
    for (int i = 0; i < length; i++)
    {
        log_i("%c",(char)payload[i]); // 打印主题内容
    }
    // if (run_data->send_cnt > 0) //已经手动发送过了
    // {
    //     archer_set_sr_type(HEART);
    // }
    // else
    // {
    //     archer_set_sr_type(RECV);
    // }
    archer_set_sr_type(HEART);
    run_data->lastHeartUpdataTime = GET_SYS_MILLIS();
    /* 亮一下 */
    // RgbParam rgb_setting = {LED_MODE_RGB,
    //                     0, 0, 0,
    //                     3,36,86,
    //                     1, 1, 1,
    //                     0.15, 0.25,
    //                     0.001, 8};
    // rgb_task_run(&rgb_setting);
    run_data->recv_cnt++;
}

ArcherAppForeverData hb_cfg;

static void write_config(ArcherAppForeverData *cfg)
{
    char tmp[32];
    String w_data;

    memset(tmp, 0, 32);
    snprintf(tmp, 16, "%d\n", cfg->role);
    w_data += tmp;

    if (cfg->role == 0)
    {
        snprintf(cfg->liz_mqtt_subtopic, 32, "AIO-beat-%s", cfg->subtopic);
        snprintf(cfg->liz_mqtt_pubtopic, 32, "AIO-heart-%s", cfg->client_id);
    }
    else
    {
        snprintf(cfg->liz_mqtt_subtopic, 32, "AIO-heart-%s", cfg->subtopic);
        snprintf(cfg->liz_mqtt_pubtopic, 32, "AIO-beat-%s", cfg->client_id);
    }
    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%s\n", cfg->liz_mqtt_subtopic);
    w_data += tmp;
    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%s\n", cfg->liz_mqtt_pubtopic);
    w_data += tmp;

    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%s\n", cfg->client_id);
    w_data += tmp;

    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%s\n", cfg->subtopic);
    w_data += tmp;

    w_data = w_data + cfg->mqtt_server.toString() + "\n";

    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%u\n", cfg->port);
    w_data += tmp;

    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%s\n", cfg->server_user);
    w_data += tmp;

    memset(tmp, 0, 32);
    snprintf(tmp, 32, "%s\n", cfg->server_password);
    w_data += tmp;

    g_flashCfg.writeFile(ARCHER_CONFIG_PATH, w_data.c_str());
}

static void read_config(ArcherAppForeverData *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以APP名为开头 以".cfg"结尾，以免多个APP读取混乱
    char info[256] = {0};
    uint16_t size = g_flashCfg.readFile(ARCHER_CONFIG_PATH, (uint8_t *)info);
    log_i("size %d\n", size);
    info[size] = 0;
    if (size == 0)
    {
        // 设置了mqtt服务器才能运行！
        log_i("Please config mqtt first!");
        // 默认值
        cfg->role = 0; // 角色
        strcpy(cfg->client_id, "");
        strcpy(cfg->subtopic, "");
        cfg->mqtt_server.fromString(DEFALUT_MQTT_IP);
        cfg->port = DEFALUT_MQTT_PORT; // mqtt服务端口
        strcpy(cfg->server_user, "");
        strcpy(cfg->server_password, "");
        write_config(cfg);
    }
    else
    {
        // 解析数据
        char *param[9] = {0};
        analyseParam(info, 9, param);
        cfg->role = atoi(param[0]);
        log_i(ARCHER_APP_NAME " role %d\n", cfg->role);

        strcpy(cfg->liz_mqtt_subtopic, param[1]);
        log_i("mqtt_subtopic %s\n", cfg->liz_mqtt_subtopic);

        strcpy(cfg->liz_mqtt_pubtopic, param[2]);
        log_i("mqtt_pubtopic %s\n", cfg->liz_mqtt_pubtopic);

        strcpy(cfg->client_id, param[3]);
        log_i("mqtt_client_id %s\n", cfg->client_id);

        strcpy(cfg->subtopic, param[4]);
        log_i("mqtt_subtopic %s\n", cfg->subtopic);

        cfg->mqtt_server.fromString(param[5]);
        log_i("mqtt_server %s\n", cfg->mqtt_server.toString().c_str());

        cfg->port = atol(param[6]);
        log_i("mqtt_port %u\n", cfg->port);

        strcpy(cfg->server_user, param[7]);
        log_i("mqtt_server_user %s\n", cfg->server_user);

        strcpy(cfg->server_password, param[8]);
        log_i("mqtt_server_password %s\n", cfg->server_password);
    }

    if (!strcmp(cfg->mqtt_server.toString().c_str(), DEFALUT_MQTT_IP) || !strcmp(cfg->mqtt_server.toString().c_str(), DEFALUT_MQTT_IP_CLIMBL) || !strcmp(cfg->mqtt_server.toString().c_str(), DEFALUT_MQTT_IP_CLIMBSNAIL))
    {
        snprintf(cfg->connect_client_id, 64, "AIO-%llu-%s", ESP.getEfuseMac(), cfg->client_id);
    }
    else
    {
        snprintf(cfg->connect_client_id, 64, "%s", cfg->client_id);
    }
    log_i("connect_client_id : %s\n", cfg->connect_client_id);
}

void ArcherAppForeverData::mqtt_reconnect()
{
    log_i("Attempting MQTT connection...\n");
    if (NULL == mqtt_client)
    {
        log_i("MQTT Client Error!\n");
        return;
    }
    char *user = NULL;
    char *password = NULL;
    // Attempt to connect
    if (strcmp(server_user, ""))
    {
        user = server_user;
        password = server_password;
    }

    if (mqtt_client->connect(connect_client_id, user, password))
    {
        log_i("mqtt connected");
        // 连接成功时订阅主题
        mqtt_client->subscribe(liz_mqtt_subtopic);
        log_i("%s subcribed\n", liz_mqtt_subtopic);
    }
    else
    {
        log_i("failed, rc=%d\n", mqtt_client->state());
    }
}

static int archer_init(AppController *sys)
{
    LVGL_OPERATE_LOCK(archer_gui_init();)
    // 初始化运行时参数
    run_data = (HeartbeatAppRunData *)calloc(1, sizeof(HeartbeatAppRunData));
    run_data->send_cnt = 0;
    run_data->recv_cnt = 0;
    run_data->timeUpdataInterval = 5000; // 日期时钟更新的时间间隔30000(30s)
    // run_data->preUpdataMillis = GET_SYS_MILLIS() - run_data->timeUpdataInterval;
    run_data->preUpdataMillis = GET_SYS_MILLIS();
    run_data->heartContinueMillis = 10000; // 心跳的持续时间10s
    // 上次心跳的更新时间
    run_data->lastHeartUpdataTime = GET_SYS_MILLIS() - run_data->heartContinueMillis;

    // 初始化MQTT
    char info[128] = {0};
    uint16_t size = g_flashCfg.readFile(ARCHER_CONFIG_PATH, (uint8_t *)info);
    if (size != 0) // 如果已经设置过heartbeat了，则开启mqtt客户端
    {
        // 获取配置参数
        read_config(&hb_cfg);
        if (NULL == hb_cfg.mqtt_client)
        {
            hb_cfg.mqtt_client = new PubSubClient(hb_cfg.mqtt_server, hb_cfg.port, hb_cfg.callback, hb_cfg.espClient);
            // hb_cfg.mqtt_client = new PubSubClient(DEFALUT_MQTT_IP_CLIMBL, hb_cfg.port, hb_cfg.callback, hb_cfg.espClient);
        }
        // 连接wifi，并开启mqtt客户端
        sys->send_to(ARCHER_APP_NAME, WIFI_SYS_NAME, APP_MESSAGE_WIFI_STA, NULL, NULL);
    }
    return 0;
}

static void archer_process(AppController *sys,
                           const ImuAction *act_info)
{
    if (DOWN_MORE == act_info->active)
    {
        sys->app_exit(); // 退出APP
        return;
    }
    else if (DOWN_MORE == act_info->active) // 向前按发送一条消息
    {
        run_data->send_cnt += 1;
        if (NULL != hb_cfg.mqtt_client)
        {
            hb_cfg.mqtt_client->publish(hb_cfg.liz_mqtt_pubtopic, "hello!");
            log_i(ARCHER_APP_NAME " sent publish %s successful\n", hb_cfg.liz_mqtt_pubtopic);
        }

        // 发送指示灯
        // RgbParam rgb_setting = {LED_MODE_RGB,
        //                     0, 0, 0,
        //                     240,240,240,
        //                     60, 60, 60,
        //                     0.15, 0.25,
        //                     0.001, 4};
        // rgb_task_run(&rgb_setting);
    }

    if (run_data->recv_cnt > 0 && run_data->send_cnt > 0)
    {
        archer_set_sr_type(HEART);
    }
    else if (run_data->recv_cnt > 0)
    {
        archer_set_sr_type(RECV);
    }
    else if (run_data->send_cnt == 0) // 进入app时自动发送mqtt消息
    {
        archer_set_sr_type(SEND);
        run_data->send_cnt += 1;
        if (NULL != hb_cfg.mqtt_client)
        {
            hb_cfg.mqtt_client->publish(hb_cfg.liz_mqtt_pubtopic, MQTT_SEND_MSG);
            log_i(ARCHER_APP_NAME " sent publish %s successful\n", hb_cfg.liz_mqtt_pubtopic);
        }
    }

    if (GET_SYS_MILLIS() - run_data->lastHeartUpdataTime >= run_data->heartContinueMillis)
    {
        // 用于停止heart
        archer_set_sr_type(SEND);
    }

    if (NULL != hb_cfg.mqtt_client)
    {
        // 以下减少网络请求的压力
        if (doDelayMillisTime(run_data->timeUpdataInterval, &run_data->preUpdataMillis, false))
        {
            // 发送请求。如果是wifi相关的消息，
            // 当请求完成后自动会调用 archer_message_handle 函数
            sys->send_to(ARCHER_APP_NAME, WIFI_SYS_NAME,
                         APP_MESSAGE_WIFI_ALIVE, NULL, NULL);
        }
    }

    // 程序需要时可以适当加延时
    LVGL_OPERATE_LOCK(display_archer();)
    LVGL_OPERATE_LOCK(archer_set_send_recv_cnt_label(run_data->send_cnt, run_data->recv_cnt);)
    LVGL_OPERATE_LOCK(display_archer_img();)
    delay(30);
}

static int archer_exit_callback(void *param)
{
    // 释放资源
    LVGL_OPERATE_LOCK(archer_gui_del();)

    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
}

static void archer_message_handle(const char *from, const char *to,
                                  APP_MESSAGE_TYPE type, void *message,
                                  void *ext_info)
{
    // 目前主要是wifi开关类事件（用于功耗控制）
    switch (type)
    {
    case APP_MESSAGE_WIFI_STA:
    {
        log_i("MQTT keep alive");
        if (!hb_cfg.mqtt_client->connected())
        {
            hb_cfg.mqtt_reconnect();
        }
        else
        {
            hb_cfg.mqtt_client->loop(); // 开启mqtt客户端
        }
    }
    break;
    case APP_MESSAGE_WIFI_AP:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_ALIVE:
    {
        log_i("MQTT keep alive(APP_MESSAGE_WIFI_ALIVE)");
        if (!hb_cfg.mqtt_client->connected())
        {
            hb_cfg.mqtt_reconnect();
        }
        else
        {
            hb_cfg.mqtt_client->loop(); // 开启mqtt客户端
        }
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
        char *param_key = (char *)message;
        if (!strcmp(param_key, "role"))
        {
            snprintf((char *)ext_info, 32, "%d", hb_cfg.role);
        }
        else if (!strcmp(param_key, "client_id"))
        {
            snprintf((char *)ext_info, 32, "%s", hb_cfg.client_id);
        }
        else if (!strcmp(param_key, "subtopic"))
        {
            snprintf((char *)ext_info, 32, "%s", hb_cfg.subtopic);
        }
        else if (!strcmp(param_key, "mqtt_server"))
        {
            snprintf((char *)ext_info, 32, "%s", hb_cfg.mqtt_server.toString().c_str());
        }
        else if (!strcmp(param_key, "port"))
        {
            snprintf((char *)ext_info, 32, "%u", hb_cfg.port);
        }
        else if (!strcmp(param_key, "server_user"))
        {
            snprintf((char *)ext_info, 32, "%s", hb_cfg.server_user);
        }
        else if (!strcmp(param_key, "server_password"))
        {
            snprintf((char *)ext_info, 32, "%s", hb_cfg.server_password);
        }
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
        char *param_key = (char *)message;
        char *param_val = (char *)ext_info;
        if (!strcmp(param_key, "role"))
        {
            hb_cfg.role = atol(param_val);
            log_i("hb role %d", hb_cfg.role);
            log_i();
        }
        else if (!strcmp(param_key, "client_id"))
        {
            snprintf((char *)hb_cfg.client_id, 32, "%s", param_val);
            log_i("mqtt_client_id %s\n", hb_cfg.client_id);
        }
        else if (!strcmp(param_key, "subtopic"))
        {
            snprintf((char *)hb_cfg.subtopic, 32, "%s", param_val);
            log_i("mqtt_subtopic %s\n", hb_cfg.subtopic);
        }
        else if (!strcmp(param_key, "mqtt_server"))
        {
            hb_cfg.mqtt_server.fromString(param_val);
            log_i("mqtt_server %s\n", hb_cfg.mqtt_server.toString().c_str());
        }
        else if (!strcmp(param_key, "port"))
        {
            hb_cfg.port = atol(param_val);
            log_i("mqtt_port %u\n", hb_cfg.port);
        }
        else if (!strcmp(param_key, "server_user"))
        {
            snprintf((char *)hb_cfg.server_user, 16, "%s", param_val);
            log_i("mqtt_server %s\n", param_val);
        }
        else if (!strcmp(param_key, "server_password"))
        {
            snprintf((char *)hb_cfg.server_password, 16, "%s", param_val);
            log_i("mqtt_server %s\n", param_val);
        }
    }
    break;
    case APP_MESSAGE_READ_CFG:
    {
        read_config(&hb_cfg);
    }
    break;
    case APP_MESSAGE_WRITE_CFG:
    {
        write_config(&hb_cfg);
    }
    break;
    default:
        break;
    }
}

APP_OBJ archer_app = {ARCHER_APP_NAME, &app_archer, "Author WoodwindHu\nVersion 2.0.0\n",
                      archer_init, archer_process,
                      archer_exit_callback, archer_message_handle};
