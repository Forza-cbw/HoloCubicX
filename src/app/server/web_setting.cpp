
// 参考代码 https://github.com/G6EJD/ESP32-8266-File-Upload

#include "network.h"
#include "common.h"
#include "app/app_name.h"
#include "server.h"
#include "web_setting.h"
#include "app/app_conf.h"
#include "FS.h"
#include <esp32-hal.h>

boolean sd_present = true;
String webpage = "";
String webpage_header = "";
String webpage_footer = "";

void Send_HTML(const String &content)
{
    log_i("Send_HTML()");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.send(200, "text/html", "");
    server.sendContent(webpage_header);

    server.sendContent(content);
    server.sendContent(webpage_footer);

    server.sendContent("");
    server.client().stop(); // Stop is needed because no content length was sent
}

String file_size(int bytes)
{
    String fsize = "";
    if (bytes < 1024)
        fsize = String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        fsize = String(bytes / 1024.0, 3) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
    else
        fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
    return fsize;
}

#define SETING_CSS ".input {display: block;margin-top: 10px;}"                                          \
                   ".input span {width: 300px;float: left;float: left;height: 36px;line-height: 36px;}" \
                   ".input input {height: 30px;width: 200px;}"                                          \
                   ".input .radio {height: 30px;width: 50px;}"                                          \
                   ".btn {width: 120px;height: 35px;background-color: #000000;border: 0px;color: #ffffff;margin-top: 15px;margin-left: auto;}" // margin-left: 100px;

#define SYS_SETTING "<form method=\"GET\" action=\"saveSysConf\">"                                                                              \
                    "<label class=\"input\"><span>WiFi SSID_0(2.4G)</span><input type=\"text\"name=\"ssid_0\"value=\"%s\"></label>"             \
                    "<label class=\"input\"><span>WiFi Passwd_0</span><input type=\"text\"name=\"password_0\"value=\"%s\"></label>"             \
                    "<label class=\"input\"><span>功耗控制（0低发热 1性能优先）</span><input type=\"text\"name=\"power_mode\"value=\"%s\"></label>"  \
                    "<label class=\"input\"><span>屏幕亮度 (值为0~100)</span><input type=\"text\"name=\"back_light\"value=\"%s\"></label>"        \
                    "<label class=\"input\"><span>屏保亮度 (值为0~100)</span><input type=\"text\"name=\"back_light2\"value=\"%s\"></label>"       \
                    "<label class=\"input\"><span>屏保触发时间（毫秒，0不使用屏保）</span><input type=\"text\"name=\"screensaver_interval\"value=\"%s\"></label>"  \
                    "<label class=\"input\"><span>屏幕方向 (0~5可选)</span><input type=\"text\"name=\"rotation\"value=\"%s\"></label>"            \
                    "<label class=\"input\"><span>操作方向（0~15可选）</span><input type=\"text\"name=\"mpu_order\"value=\"%s\"></label>"          \
                    "<label class=\"input\"><span>MPU6050自动校准</span><input class=\"radio\" type=\"radio\" value=\"0\" name=\"auto_calibration_mpu\" %s>关闭<input class=\"radio\" type=\"radio\" value=\"1\" name=\"auto_calibration_mpu\" %s>开启</label>" \
                    "<label class=\"input\"><span>开机自启的APP名字</span><input type=\"text\"name=\"auto_start_app\"value=\"%s\"></label>"        \
                    "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define RGB_SETTING "<form method=\"GET\" action=\"saveRgbConf\">"                                                                                             \
                    "<label class=\"input\"><span>RGB最低亮度（0.00~0.99可选）</span><input type=\"text\"name=\"min_brightness\"value=\"%s\"></label>" \
                    "<label class=\"input\"><span>RGB最高亮度（0.00~0.99可选）</span><input type=\"text\"name=\"max_brightness\"value=\"%s\"></label>"                 \
                    "<label class=\"input\"><span>RGB亮度步长（0.00~0.99可选）</span><input type=\"text\"name=\"brightness_step\"value=\"%s\"></label>"                \
                    "<label class=\"input\"><span>RGB渐变时间（整数毫秒值）</span><input type=\"text\"name=\"time\"value=\"%s\"></label>"           \
                    "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define WEATHER_SETTING "<form method=\"GET\" action=\"saveWeatherConf\">"                                                                                          \
                        "<label class=\"input\"><span>高德天气城市编码[身份证前六位]</span><input type=\"text\"name=\"CITY_CODE\"value=\"%s\"></label>"                               \
                        "<label class=\"input\"><span>高德天气API[自己注册]</span><input type=\"text\"name=\"WEATHER_API_KEY\"value=\"%s\"></label>"                     \
                        "<label class=\"input\"><span>天气更新周期（毫秒）</span><input type=\"text\"name=\"weatherUpdataInterval\"value=\"%s\"></label>" \
                        "<label class=\"input\"><span>日期更新周期（毫秒）</span><input type=\"text\"name=\"timeUpdataInterval\"value=\"%s\"></label>"    \
                        "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define PICTURE_SETTING "<form method=\"GET\" action=\"savePictureConf\">"                                                                                         \
                        "<label class=\"input\"><span>自动切换时间间隔（毫秒）</span><input type=\"text\"name=\"switchInterval\"value=\"%s\"></label>" \
                        "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define MEDIA_SETTING "<form method=\"GET\" action=\"saveMediaConf\">"                                                                                             \
                      "<label class=\"input\"><span>自动切换（0不切换 1自动切换）</span><input type=\"text\"name=\"switchFlag\"value=\"%s\"></label>" \
                      "<label class=\"input\"><span>功耗控制（0低发热 1性能优先）</span><input type=\"text\"name=\"powerFlag\"value=\"%s\"></label>"  \
                      "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define ARCHER_SETTING "<form method=\"GET\" action=\"saveArcherConf\">"                                                                            \
                          "<label class=\"input\"><span>Role(0:heart,1:beat)</span><input type=\"text\"name=\"role\"value=\"%s\"></label>"                \
                          "<label class=\"input\"><span>MQTT ClientID(推荐QQ号)</span><input type=\"text\"name=\"mqtt_client_id\"value=\"%s\"></label>"           \
                          "<label class=\"input\"><span>MQTT SubTopic(推荐对方QQ号)</span><input type=\"text\"name=\"mqtt_subtopic\"value=\"%s\"></label>"         \
                        "<label class=\"input\"><span>MQTT ServerIp</span><input type=\"text\"name=\"mqtt_server\"value=\"%s\"></label>"                    \
                        "<label class=\"input\"><span>MQTT 端口号(1883)</span><input type=\"text\"name=\"mqtt_port\"value=\"%s\"></label>"             \
                        "<label class=\"input\"><span>MQTT 服务用户名(可不填)</span><input type=\"text\"name=\"mqtt_user\"value=\"%s\"></label>"  \
                        "<label class=\"input\"><span>MQTT 服务密码(可不填)</span><input type=\"text\"name=\"mqtt_password\"value=\"%s\"></label>" \
                        "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"


#define REMOTR_SENSOR_SETTING "<form method=\"GET\" action=\"savePCResourceConf\">"                                                                          \
                              "<label class=\"input\"><span>PC地址</span><input type=\"text\"name=\"pc_ipaddr\"value=\"%s\"></label>"                         \
                              "<label class=\"input\"><span>传感器数据更新间隔(ms)</span><input type=\"text\"name=\"sensorUpdataInterval\"value=\"%s\"></label>" \
                              "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

void init_page_header()
{
    webpage_header = F("<!DOCTYPE html><html>");
    webpage_header += F("<head>");
    webpage_header += F("<title>HoloCubic WebServer</title>"); // NOTE: 1em = 16px
    webpage_header += F("<meta http-equiv='Content-Type' name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width; text/html; charset=utf-8' />");
    webpage_header += F("<style>");
    webpage_header += F(SETING_CSS);
    webpage_header += F("body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#dbdadb;}");
    webpage_header += F("ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#878588;font-size:1em;}");
    webpage_header += F("li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}");
    webpage_header += F("li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}");
    webpage_header += F("li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}");
    webpage_header += F("section {font-size:0.88em;}");
    webpage_header += F("h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#4c4c4d;}");
    webpage_header += F("h2{color:orange;font-size:1.0em;}");
    webpage_header += F("h3{font-size:0.8em;}");
    webpage_header += F("table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}");
    webpage_header += F("th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}");
    webpage_header += F("tr:nth-child(odd) {background-color:#eeeeee;}");
    webpage_header += F(".rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}");
    webpage_header += F(".rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}");
    webpage_header += F(".rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}");
    webpage_header += F(".column{float:left;width:50%;height:45%;}");
    webpage_header += F(".row:after{content:'';display:table;clear:both;}");
    webpage_header += F("*{box-sizing:border-box;}");
    webpage_header += F("footer{background-color:#b1b1b1; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}");
    webpage_header += F("button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}");
    webpage_header += F(".buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}");
    webpage_header += F(".buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}");
    webpage_header += F(".buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}");
    webpage_header += F(".buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}");
    webpage_header += F("a{font-size:75%;}");
    webpage_header += F("p{font-size:75%;}");

    webpage_header += F("</style></head><body>");

    webpage_header += F("<h1>HoloCubic_AIO ");
    webpage_header += F(AIO_VERSION "</h1>");
    webpage_header += F("<ul>");
    webpage_header += F("<li><a href='/'>Home</a></li>"); // Lower Menu bar command entries
    webpage_header += F("<li><a href='/download'>Download</a></li>");
    webpage_header += F("<li><a href='/upload'>Upload</a></li>");
    webpage_header += F("<li><a href='/delete'>Delete</a></li>");

    webpage_header += F("<li><a href='/sys_setting'>系统设置</a></li>");

    webpage_header += F("<li><a href='/rgb_setting'>RGB设置</a></li>");
#if APP_WEATHER_USE
    webpage_header += F("<li><a href='/weather_setting'>新版天气</a></li>");
#endif
#if APP_PICTURE_USE
    webpage_header += F("<li><a href='/picture_setting'>相册</a></li>");
#endif
#if APP_MEDIA_PLAYER_USE
    webpage_header += F("<li><a href='/media_setting'>媒体播放器</a></li>");
#endif

#if APP_ARCHER_USE
    webpage_header += F("<li><a href='/archer_setting'>丘比特</a></li>");
#endif
#if APP_PC_RESOURCE_USE
    webpage_header += F("<li><a href='/pc_resource_setting'>PC资源监控</a></li>");
#endif
    webpage_header += F("</ul>");
}

void init_page_footer()
{
    webpage_footer = F("<footer>&copy;ClimbSnail 2021</footer>");
    webpage_footer += F("</body></html>");
}

// All supporting functions from here...
void HomePage()
{
    // 指定 target='_blank' 设置新建页面
    webpage = F("<a href='https://github.com/ClimbSnail/HoloCubic_AIO' target='_blank'><button>Github</button></a>");
    webpage += F("<a href='https://space.bilibili.com/344470052?spm_id_from=333.788.b_765f7570696e666f.1' target='_blank'><button>BiliBili教程</button></a>");
    Send_HTML(webpage);
}

void sys_setting()
{
    char buf[2048];
    char ssid_0[32];
    char password_0[32];
    char power_mode[32];
    char back_light[32];
    char back_light2[32];
    char screensaver_interval[32];
    char rotation[32];
    char mpu_order[32];
    char min_brightness[32];
    char max_brightness[32];
    char time[32];
    char auto_calibration_mpu[32];
    char auto_start_app[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"ssid_0", ssid_0, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"password_0", password_0, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"power_mode", power_mode, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"back_light", back_light, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"back_light2", back_light2, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"screensaver_interval", screensaver_interval, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"rotation", rotation, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"mpu_order", mpu_order, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"min_brightness", min_brightness, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"max_brightness", max_brightness, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"time", time, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"auto_calibration_mpu", auto_calibration_mpu, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"auto_start_app", auto_start_app, true);
    SysUtilConfig cfg = app_controller->sys_cfg;
    // 主要为了处理启停MPU自动校准的单选框
    if (0 == cfg.auto_calibration_mpu)
    {
        sprintf(buf, SYS_SETTING,
                ssid_0, password_0,
                power_mode, back_light, back_light2, screensaver_interval, rotation,
                mpu_order, "checked=\"checked\"", "",
                auto_start_app);
    }
    else
    {
        sprintf(buf, SYS_SETTING,
                ssid_0, password_0,
                power_mode, back_light, back_light2, screensaver_interval, rotation,
                mpu_order, "", "checked=\"checked\"",
                auto_start_app);
    }
    webpage = buf;
    Send_HTML(webpage);
}

void rgb_setting()
{
    char buf[2048];
    char min_brightness[32];
    char max_brightness[32];
    char brightness_step[32];
    char time[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"min_brightness", min_brightness, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"max_brightness", max_brightness, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"brightness_step", brightness_step, true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"time", time, true);
    sprintf(buf, RGB_SETTING,
            min_brightness, max_brightness, brightness_step, time);
    webpage = buf;
    Send_HTML(webpage);
}

void weather_setting()
{
    char buf[2048];
    char tianqi_url[128];
    char CITY_CODE[32];
    char WEATHER_API_KEY[33];
    char tianqi_addr[32];
    char weatherUpdataInterval[32];
    char timeUpdataInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"tianqi_url", tianqi_url, true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"CITY_CODE", CITY_CODE, true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"WEATHER_API_KEY", WEATHER_API_KEY, true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"tianqi_addr", tianqi_addr, true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"weatherUpdataInterval", weatherUpdataInterval, true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"timeUpdataInterval", timeUpdataInterval, true);
    sprintf(buf, WEATHER_SETTING, CITY_CODE,
            WEATHER_API_KEY,
            weatherUpdataInterval,
            timeUpdataInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void picture_setting()
{
    char buf[2048];
    char switchInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, PICTURE_APP_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);
    app_controller->send_to(SERVER_APP_NAME, PICTURE_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"switchInterval", switchInterval, true);
    sprintf(buf, PICTURE_SETTING, switchInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void media_setting()
{
    char buf[2048];
    char switchFlag[32];
    char powerFlag[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, MEDIA_PLAYER_APP_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);
    app_controller->send_to(SERVER_APP_NAME, MEDIA_PLAYER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"switchFlag", switchFlag, true);
    app_controller->send_to(SERVER_APP_NAME, MEDIA_PLAYER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"powerFlag", powerFlag, true);
    sprintf(buf, MEDIA_SETTING, switchFlag, powerFlag);
    webpage = buf;
    Send_HTML(webpage);
}

void archer_setting()
{
    char buf[2048];
    char role[32];
    char client_id[32];
    char subtopic[32];
    char mqtt_server[32];
    char port[32];
    char server_user[32];
    char server_password[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);

    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"role", role, true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"client_id", client_id, true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"subtopic", subtopic, true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"mqtt_server", mqtt_server, true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"port", port, true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"server_user", server_user, true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"server_password", server_password, true);

    sprintf(buf, ARCHER_SETTING, role, client_id, subtopic, mqtt_server,
            port, server_user, server_password);
    webpage = buf;
    Send_HTML(webpage);
}

void pc_resource_setting()
{
    char buf[2048];
    char pc_ipaddr[32];
    char sensorUpdataInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, PC_RESOURCE_APP_NAME, APP_MESSAGE_READ_CFG,
                            NULL, NULL, true);
    app_controller->send_to(SERVER_APP_NAME, PC_RESOURCE_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"pc_ipaddr", pc_ipaddr, true);
    app_controller->send_to(SERVER_APP_NAME, PC_RESOURCE_APP_NAME, APP_MESSAGE_GET_PARAM,
                            (void *)"sensorUpdataInterval", sensorUpdataInterval, true);
    sprintf(buf, REMOTR_SENSOR_SETTING, pc_ipaddr, sensorUpdataInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void saveSysConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"ssid_0",
                            (void *)server.arg("ssid_0").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"password_0",
                            (void *)server.arg("password_0").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"power_mode",
                            (void *)server.arg("power_mode").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"back_light",
                            (void *)server.arg("back_light").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"back_light2",
                            (void *)server.arg("back_light2").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"screensaver_interval",
                            (void *)server.arg("screensaver_interval").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"rotation",
                            (void *)server.arg("rotation").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"mpu_order",
                            (void *)server.arg("mpu_order").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"auto_calibration_mpu",
                            (void *)server.arg("auto_calibration_mpu").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"auto_start_app",
                            (void *)server.arg("auto_start_app").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void saveRgbConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"min_brightness",
                            (void *)server.arg("min_brightness").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"max_brightness",
                            (void *)server.arg("max_brightness").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"brightness_step",
                            (void *)server.arg("brightness_step").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"time",
                            (void *)server.arg("time").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, CONFIG_SYS_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void saveWeatherConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"tianqi_url",
                            (void *)server.arg("tianqi_url").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"CITY_CODE",
                            (void *)server.arg("CITY_CODE").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"WEATHER_API_KEY",
                            (void *)server.arg("WEATHER_API_KEY").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"tianqi_addr",
                            (void *)server.arg("tianqi_addr").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"weatherUpdataInterval",
                            (void *)server.arg("weatherUpdataInterval").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"timeUpdataInterval",
                            (void *)server.arg("timeUpdataInterval").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, WEATHER_APP_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void savePictureConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, PICTURE_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchInterval",
                            (void *)server.arg("switchInterval").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, PICTURE_APP_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void saveMediaConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, MEDIA_PLAYER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchFlag",
                            (void *)server.arg("switchFlag").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, MEDIA_PLAYER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"powerFlag",
                            (void *)server.arg("powerFlag").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, MEDIA_PLAYER_APP_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void saveArcherConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"role",
                            (void *)server.arg("role").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"client_id",
                            (void *)server.arg("mqtt_client_id").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"subtopic",
                            (void *)server.arg("mqtt_subtopic").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"mqtt_server",
                            (void *)server.arg("mqtt_server").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"port",
                            (void *)server.arg("mqtt_port").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"server_user",
                            (void *)server.arg("mqtt_user").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"server_password",
                            (void *)server.arg("mqtt_password").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, ARCHER_APP_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void savePCResourceConf(void)
{
    app_controller->send_to(SERVER_APP_NAME, PC_RESOURCE_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"pc_ipaddr",
                            (void *)server.arg("pc_ipaddr").c_str(), true);
    app_controller->send_to(SERVER_APP_NAME, PC_RESOURCE_APP_NAME,
                            APP_MESSAGE_SET_PARAM,
                            (void *)"sensorUpdataInterval",
                            (void *)server.arg("sensorUpdataInterval").c_str(), true);
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, PC_RESOURCE_APP_NAME, APP_MESSAGE_WRITE_CFG,
                            NULL, NULL, true);

    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
}

void File_Delete()
{
    Send_HTML(
        F("<h3>Enter filename to delete</h3>"
          "<form action='/delete_result' method='post'>"
          "<input type='text' name='delete_filepath' placeHolder='绝对路径 /image/...'><br>"
          "</label><input class=\"btn\" type=\"submit\" name=\"Submie\" value=\"确认删除\"></form>"
          "<a href='/'>[Back]</a>"));
}

void delete_result(void)
{
    String del_file = server.arg("delete_filepath");
    boolean ret = tf.deleteFile(del_file);
    if (ret)
    {
        webpage = "<h3>Delete succ!</h3><a href='/delete'>[Back]</a>";
        tf.listDir("/image", 250);
    }
    else
    {
        webpage = "<h3>Delete fail! Please check up file path.</h3><a href='/delete'>[Back]</a>";
    }
    tf.listDir("/image", 250);
    Send_HTML(webpage);
}

void File_Download()
{ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
    if (server.args() > 0)
    { // Arguments were received
        if (server.hasArg("download"))
            sd_file_download(server.arg(0));
    }
    else
        SelectInput("Enter filename to download", "download", "download");
}

void sd_file_download(const String &filename)
{
    if (sd_present)
    {
        File download = tf.open("/" + filename);
        if (download)
        {
            server.sendHeader("Content-Type", "text/text");
            server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
            server.sendHeader("Connection", "close");
            server.streamFile(download, "application/octet-stream");
            download.close();
        }
        else
            ReportFileNotPresent(String("download"));
    }
    else
        ReportSDNotPresent();
}

void File_Upload()
{
    tf.listDir("/image", 250);

    webpage = webpage_header;
    webpage += "<h3>Select File to Upload</h3>"
               "<FORM action='/fupload' method='post' enctype='multipart/form-data'>"
               "<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>"
               "<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>"
               "<a href='/'>[Back]</a><br><br>";
    webpage += webpage_footer;
    server.send(200, "text/html", webpage);
}

File UploadFile;
void handleFileUpload()
{                                                   // upload a new file to the Filing system
    HTTPUpload &uploadFileStream = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                                    // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
    String filename = uploadFileStream.filename;
    if (uploadFileStream.status == UPLOAD_FILE_START)
    {
        // String filename = uploadFileStream.filename;
        // if (!filename.startsWith("/image"))
        filename = "/image/" + filename;
        log_i("Upload File Name: s", filename.c_str());
        tf.deleteFile(filename);                    // Remove a previous version, otherwise data is appended the file again
        UploadFile = tf.open(filename, FILE_WRITE); // Open the file for writing in SPIFFS (create it, if doesn't exist)
    }
    else if (uploadFileStream.status == UPLOAD_FILE_WRITE)
    {
        if (UploadFile)
            UploadFile.write(uploadFileStream.buf, uploadFileStream.currentSize); // Write the received bytes to the file
    }
    else if (uploadFileStream.status == UPLOAD_FILE_END)
    {
        if (UploadFile) // If the file was successfully created
        {
            UploadFile.close(); // Close the file again
            log_i("Upload Size: %d", uploadFileStream.totalSize);
            webpage = webpage_header;
            webpage += F("<h3>File was successfully uploaded</h3>");
            webpage += F("<h2>Uploaded File Name: ");
            webpage += filename + "</h2>";
            webpage += F("<h2>File Size: ");
            webpage += file_size(uploadFileStream.totalSize) + "</h2><br>";
            webpage += webpage_footer;
            server.send(200, "text/html", webpage);
            tf.listDir("/image", 250);
        }
        else
        {
            ReportCouldNotCreateFile(String("upload"));
        }
    }
}

void SelectInput(String heading, String command, String arg_calling_name)
{
    webpage = F("<h3>");
    webpage += heading + "</h3>";
    webpage += F("<FORM action='/");
    webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
    webpage += F("<input type='text' name='");
    webpage += arg_calling_name;
    webpage += F("' value=''><br>");
    webpage += F("<type='submit' name='");
    webpage += arg_calling_name;
    webpage += F("' value=''><br>");
    webpage += F("<a href='/'>[Back]</a>");
    Send_HTML(webpage);
}

void ReportSDNotPresent()
{
    webpage = F("<h3>No SD Card present</h3>");
    webpage += F("<a href='/'>[Back]</a><br><br>");
    Send_HTML(webpage);
}

void ReportFileNotPresent(const String &target)
{
    webpage = F("<h3>File does not exist</h3>");
    webpage += F("<a href='/");
    webpage += target + "'>[Back]</a><br><br>";
    Send_HTML(webpage);
}

void ReportCouldNotCreateFile(const String &target)
{
    webpage = F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
    webpage += F("<a href='/");
    webpage += target + "'>[Back]</a><br><br>";
    Send_HTML(webpage);
}