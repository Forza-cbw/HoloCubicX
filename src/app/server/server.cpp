#include "server.h"
#include "server_gui.h"
#include "web_setting.h"
#include "sys/app_controller.h"
#include "app/app_conf.h"
#include "network.h"
#include "common.h"
#include "gui_lock.h"

#define SERVER_REFLUSH_INTERVAL 5000UL // 配置界面重新刷新时间(5s)
#define DNS_PORT 53                    // DNS端口
WebServer server(80);

// DNSServer dnsServer;

struct ServerAppRunData
{
    boolean web_start;                    // 标志是否开启web server服务，0为关闭 1为开启
    boolean req_sent;                     // 标志是否发送wifi请求服务，0为关闭 1为开启
    unsigned long serverReflushPreMillis; // 上一回更新的时间
    AppController *sys = NULL;            // 退出时关闭ap
};

static ServerAppRunData *run_data = NULL;

void start_web_config()
{
    // 首页
    server.on("/", HTTP_GET, HomePage);

    init_page_header();
    init_page_footer();
    server.on("/download", File_Download);
    server.on("/upload", File_Upload);
    server.on("/delete", File_Delete);
    server.on("/delete_result", delete_result);

    server.on("/sys_setting", sys_setting);
    server.on("/rgb_setting", rgb_setting);
#if APP_WEATHER_USE
    server.on("/weather_setting", weather_setting);
#endif
#if APP_PICTURE_USE
    server.on("/picture_setting", picture_setting);
#endif
#if APP_MEDIA_PLAYER_USE
    server.on("/media_setting", media_setting);
#endif
#if APP_ARCHER_USE
    server.on("/archer_setting", archer_setting);
#endif
#if APP_PC_RESOURCE_USE
    server.on("/pc_resource_setting", pc_resource_setting);
#endif

    server.on(
        "/fupload", HTTP_POST,
        []()
        { server.send(200); },
        handleFileUpload);

    // 连接
    server.on("/saveSysConf", saveSysConf);
    server.on("/saveRgbConf", saveRgbConf);
#if APP_WEATHER_USE
    server.on("/saveWeatherConf", saveWeatherConf);
#endif
#if APP_PICTURE_USE
    server.on("/savePictureConf", savePictureConf);
#endif
#if APP_MEDIA_PLAYER_USE
    server.on("/saveMediaConf", saveMediaConf);
#endif
#if APP_ARCHER_USE
    server.on("/saveArcherConf", saveArcherConf);
#endif
#if APP_PC_RESOURCE_USE
    server.on("/savePCResourceConf", savePCResourceConf);
#endif

    server.begin();
    // MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");

    // dnsServer.start(DNS_PORT, "*", gateway);
}

void stop_web_config()
{
    run_data->web_start = 0;
    run_data->req_sent = 0;
    server.stop();
    server.close();
}

static int server_init(AppController *sys)
{
    LVGL_OPERATE_LOCK(server_gui_init();)
    // 初始化运行时参数
    run_data = (ServerAppRunData *)malloc(sizeof(ServerAppRunData));
    run_data->web_start = 0;
    run_data->req_sent = 0;
    run_data->serverReflushPreMillis = 0;
    run_data->sys = sys;
    return 0;
}

static void server_process(AppController *sys,
                           const ImuAction *action)
{
    lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;

    if (DOWN_MORE == action->active)
    {
        stop_web_config();
        sys->app_exit();
        return;
    }

    if (0 == run_data->web_start && 0 == run_data->req_sent)
    {
        // 预显示
        LVGL_OPERATE_LOCK(display_setting(
            "WebServer Start",
            "Domain: holocubic",
            "Wait...", "Wait...",
            // "", "",
            LV_SCR_LOAD_ANIM_NONE);)

        sys->send_to(SERVER_APP_NAME, WIFI_SYS_NAME,
                     APP_MESSAGE_WIFI_STA, NULL, NULL);
        // 如果web服务没有开启 且 ap开启的请求没有发送 message这边没有作用（填0）
        sys->send_to(SERVER_APP_NAME, WIFI_SYS_NAME,
                     APP_MESSAGE_WIFI_AP, NULL, NULL);
        run_data->req_sent = 1; // 标志为 ap开启请求已发送
    }
    else if (1 == run_data->web_start)
    {
        server.handleClient(); // 一定需要放在循环里扫描
        // dnsServer.processNextRequest();
        if (doDelayMillisTime(SERVER_REFLUSH_INTERVAL, &run_data->serverReflushPreMillis, false) == true)
        {
            // 发送wifi维持的心跳
            sys->send_to(SERVER_APP_NAME, WIFI_SYS_NAME,
                         APP_MESSAGE_WIFI_ALIVE, NULL, NULL);

            LVGL_OPERATE_LOCK(display_setting(
                "WebServer Start",
                "Domain: holocubic",
                WiFi.localIP().toString().c_str(),
                WiFi.softAPIP().toString().c_str(),
                LV_SCR_LOAD_ANIM_NONE);)
        }
    }
}

static int server_exit_callback(void *param)
{
    // 关闭AP
    run_data->sys->send_to(SERVER_APP_NAME, WIFI_SYS_NAME,
                           APP_MESSAGE_WIFI_AP_CLOSE, NULL, NULL);

    LVGL_OPERATE_LOCK(setting_gui_del();)
    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }

    return 0;
}

static void server_message_handle(const char *from, const char *to,
                                  APP_MESSAGE_TYPE type, void *message,
                                  void *ext_info)
{
    switch (type)
    {
        case APP_MESSAGE_WIFI_STA:
        {
            log_i("APP_MESSAGE_WIFI_STA enable");
        }
        break;
        case APP_MESSAGE_WIFI_AP:
        {
            log_i("APP_MESSAGE_WIFI_AP enable");
            LVGL_OPERATE_LOCK(display_setting(
                "WebServer Start",
                "Domain: holocubic",
                WiFi.localIP().toString().c_str(),
                WiFi.softAPIP().toString().c_str(),
                LV_SCR_LOAD_ANIM_NONE);)
            start_web_config();
            run_data->web_start = 1;
        }
        break;
        case APP_MESSAGE_WIFI_ALIVE:
        {
        }
        break;
        default:
            break;
    }
}

APP_OBJ server_app = {SERVER_APP_NAME, &app_server, "",
                      server_init, server_process,
                      server_exit_callback, server_message_handle};
