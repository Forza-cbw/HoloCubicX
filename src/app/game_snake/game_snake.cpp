#include "game_snake.h"
#include "game_snake_gui.h"
#include "app/app_name.h"
#include "sys/app_controller.h"
#include "common.h"
#include "gui_lock.h"

// 游戏名称


#define SNAKE_SPEED 1000

struct SnakeAppRunData
{
    unsigned int score;
    int gameStatus;
};

static SnakeAppRunData *run_data = NULL;

static int game_snake_init(AppController *sys)
{
    // 随机数种子
    randomSeed(analogRead(A0));
    // 初始化运行时的参数
    LVGL_OPERATE_LOCK(game_snake_gui_init();)
    // 初始化运行时参数
    run_data = (SnakeAppRunData *)calloc(1, sizeof(SnakeAppRunData));
    run_data->score = 0;
    run_data->gameStatus = 0;

    return 0;
}

static void game_snake_process(AppController *sys, const ImuAction *act_info)
{
    if (DOWN_MORE == act_info->active)
    {
        run_data->gameStatus = -1;
        sys->app_exit(); // 退出APP
        return;
    }

    // 操作触发
    if (TURN_RIGHT == act_info->active)
    {
        update_driection(DIR_RIGHT);
    }
    else if (TURN_LEFT == act_info->active)
    {
        update_driection(DIR_LEFT);
    }
    else if (UP == act_info->active)
    {
        update_driection(DIR_UP);
    }
    else if (DOWN == act_info->active)
    {
        update_driection(DIR_DOWN);
    }

    if (run_data->gameStatus == 0)
    {
        LVGL_OPERATE_LOCK(display_snake(run_data->gameStatus, LV_SCR_LOAD_ANIM_NONE););
    }

    // 速度控制
    delay(SNAKE_SPEED);
}

static int game_snake_exit_callback(void *param)
{
    // 释放页面资源
    LVGL_OPERATE_LOCK(game_snake_gui_del();)

    // 释放事件资源
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
}

static void game_snake_message_handle(const char *from, const char *to,
                                      APP_MESSAGE_TYPE type, void *message,
                                      void *ext_info)
{
    // 目前主要是wifi开关类事件（用于功耗控制）
    switch (type)
    {
    case APP_MESSAGE_WIFI_STA:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_AP:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_ALIVE:
    {
        // wifi心跳维持的响应 可以不做任何处理
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
    }
    break;
    default:
        break;
    }
}

APP_OBJ game_snake_app = {GAME_APP_NAME, &app_game_snake, "",
                          game_snake_init, game_snake_process,
                          game_snake_exit_callback, game_snake_message_handle};
