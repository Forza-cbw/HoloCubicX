#ifndef HOLOCUBICX_APP_NAME_H
#define HOLOCUBICX_APP_NAME_H

// 他们三用getAppByName()是查不到的
#define SELF_SYS_NAME "SelfSys"
#define WIFI_SYS_NAME "WifiSys" // 最终会调用running APP的消息处理函数
#define CONFIG_SYS_NAME "ConfigSys" // 读写系统配置（读写APP配置，目的地应该是APP名）

#define ARCHER_APP_NAME "Archer"
#define EXAMPLE_APP_NAME "Example"
#define FILE_MANAGER_APP_NAME "File Manager"
#define G2048_APP_NAME "2048"
#define GAME_APP_NAME "Snake"
#define IDEA_APP_NAME "Idea"
#define APP_NAME   "LOVE"
#define MEDIA_PLAYER_APP_NAME "Media"
#define PC_RESOURCE_APP_NAME "PC Resource"
#define PICTURE_APP_NAME "Picture"
#define SETTINGS_APP_NAME "Settings"
#define TOMATO_APP_NAME "Tomato"
#define WEATHER_APP_NAME "Weather"


#endif //HOLOCUBICX_APP_NAME_H
