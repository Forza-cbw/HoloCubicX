#include "game_2048.h"
#include "game_2048_gui.h"
#include "game2048_contorller.h"
#include "app/app_name.h"
#include "sys/app_controller.h"
#include "gui_lock.h"

GAME2048 game;

struct Game2048AppRunData
{
//    int Normal = 0;       // 记录移动的方向
//    BaseType_t xReturned_task_one = pdFALSE;
//    TaskHandle_t xHandle_task_one = NULL;
//    BaseType_t xReturned_task_two = pdFALSE;
//    TaskHandle_t xHandle_task_two = NULL;
};

static Game2048AppRunData *run_data = NULL;

static int game_2048_init(AppController *sys)
{
    // 初始化运行时的参数
    LVGL_OPERATE_LOCK(game_2048_gui_init();)

    randomSeed(analogRead(25));
    // 初始化运行时参数
    run_data = (Game2048AppRunData *)calloc(1, sizeof(Game2048AppRunData));
    game.init();

    // 刷新棋盘显示
    int new1 = game.addRandom();
    int new2 = game.addRandom();
    LVGL_OPERATE_LOCK(render_board(game.getBoard());)
    // 棋子出生动画
    LVGL_OPERATE_LOCK(render_born(new1);)
    LVGL_OPERATE_LOCK(render_born(new2);)

    return 0;
}

static void game_2048_process(AppController *sys,
                              const ImuAction *act_info)
{
    int direction = -1;
    if (DOWN_MORE == act_info->active)
    {
        sys->app_exit(); // 退出APP
        return;
    }

    // 具体操作
    switch (act_info->active) {
        case UP: direction = 1; break;
        case DOWN: direction = 2; break;
        case TURN_LEFT: direction = 3; break;
        case TURN_RIGHT: direction = 4; break;
    }

    if (UNKNOWN != act_info->active) {
        game.moveAndMerge(direction);//移动且合并
        // 渲染动画
        if (game.isChanged()) { // 棋盘布局改变
            // 移动和合并动画
            showAnim(game.getMoveRecord(), game.getDstNeedZoom(), direction, game.getBoard());
            // 新数字出生动画
            showNewBorn(game.addRandom(), game.getBoard());
        }
    }

    if (game.judge() == 1)
    {
        //   rgb.setRGB(0, 255, 0);
        log_i("you win!");
    }
    else if (game.judge() == 2)
    {
        //   rgb.setRGB(255, 0, 0);
        log_i("you lose!");
    }

    // 程序需要时可以适当加延时
    delay(100);
}

static int game_2048_exit_callback(void *param)
{
//    // 查杀任务
//    if (pdPASS == run_data->xReturned_task_one)
//    {
//        vTaskDelete(run_data->xHandle_task_one);
//    }
//    if (pdPASS == run_data->xReturned_task_two)
//    {
//        vTaskDelete(run_data->xHandle_task_two);
//    }

    LVGL_OPERATE_LOCK(game_2048_gui_del();)

    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
}

static void game_2048_message_handle(const char *from, const char *to,
                                     APP_MESSAGE_TYPE type, void *message,
                                     void *ext_info)
{
    // 目前事件主要是wifi开关类事件（用于功耗控制）
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
    default:
        break;
    }
}

APP_OBJ game_2048_app = {G2048_APP_NAME, &app_game_2048, "",
                         game_2048_init, game_2048_process,
                         game_2048_exit_callback, game_2048_message_handle};
