#ifndef INTERFACE_H
#define INTERFACE_H

enum APP_MESSAGE_TYPE
{
    /* wifi相关 - 异步 */
    /* 目的 WIFI_SYS_NAME */
    APP_MESSAGE_WIFI_STA = 0, // 开启连接
    APP_MESSAGE_WIFI_ALIVE,    // wifi开关的心跳维持
    APP_MESSAGE_WIFI_AP,       // 开启AP事件
    APP_MESSAGE_WIFI_DISABLE,  // 关闭wifi(sta & ap)
    APP_MESSAGE_WIFI_AP_CLOSE, // 关闭AP

    /* 配置相关 - 同步 */
    /* 目的 WIFI_SYS_NAME | **_APP_NAME */
    APP_MESSAGE_GET_PARAM, // 获取参数
    APP_MESSAGE_SET_PARAM, // 设置参数
    APP_MESSAGE_READ_CFG,  // 向磁盘读取参数
    APP_MESSAGE_WRITE_CFG, // 向磁盘写入参数

//    APP_MESSAGE_UPDATE_TIME,
//    APP_MESSAGE_MQTT_DATA, // MQTT客户端收到消息

    APP_MESSAGE_NONE
};

enum APP_TYPE
{
    APP_TYPE_REAL_TIME = 0, // 实时应用
    APP_TYPE_BACKGROUND,    // todo 后台应用

    APP_TYPE_NONE
};

class AppController;
struct ImuAction;

struct APP_OBJ
{
    // 应用程序名称 及title
    const char *app_name;

    // APP的图片存放地址    APP应用图标 128*128
    const void *app_image;

    // 应用程序的其他信息 如作者、版本号等等
    const char *app_info;

    // APP的初始化函数 也可以为空或什么都不做（作用等效于arduino setup()函数）
    int (*app_init)(AppController *sys);

    // APP的主程序函数入口指针
    void (*main_process)(AppController *sys,
                         const ImuAction *act_info);

    // 退出之前需要处理的回调函数 可为空
    int (*exit_callback)(void *param);

    // 消息处理机制
    void (*message_handle)(const char *from, const char *to,
                           APP_MESSAGE_TYPE type, void *message,
                           void *ext_info);
};

#endif