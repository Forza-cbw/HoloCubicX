#include "tomato.h"
#include "tomato_gui.h"
#include "app/app_name.h"
#include "sys/app_controller.h"
#include "common.h"
#include "ESP32Time.h"
#include "gui_lock.h"

#define ON 1
#define OFF 0

// 动态数据，APP的生命周期结束也需要释放它
struct TomatoAppRunData
{
    unsigned long time_start; // 记录系统开始计时时的毫秒数
    unsigned long time_ms;    // 毫秒数,对应倒计时显示的时间
    TimeStr t;                // 时间结构体
    TimeStr t_start;          // 倒计时结构体
    RgbConfig rgb_cfg;        // 灯效
    bool rgb_fast;            // 使能
    bool rgb_fast_update;     // 标志位
    int time_mode;            // 倒计时种类
    uint8_t switch_count;     // 切换次数，用于消抖
    ACTIVE_TYPE lastAct;
};

// 常驻数据，可以不随APP的生命周期而释放或删除
struct TomatoAppForeverData
{
};

static bool hadOpened = false;

// 保存APP运行时的参数信息，理论上关闭APP时推荐在 xxx_exit_callback 中释放掉
static TomatoAppRunData *run_data = NULL;

// 当然你也可以添加恒定在内存中的少量变量（退出时不用释放，实现第二次启动时可以读取）
// 考虑到所有的APP公用内存，尽量减少 forever_data 的数据占用
static TomatoAppForeverData forever_data;

static int tomato_init(AppController *sys)
{
    // 初始化运行时的参数
    LVGL_OPERATE_LOCK(tomato_gui_init();)
    // 初始化运行时参数
    run_data = (TomatoAppRunData *)calloc(1, sizeof(TomatoAppRunData));
    run_data->time_start = millis();
    run_data->t_start.second = 0; // 专注时间，初始化一次
    run_data->t_start.minute = 25;
    run_data->t = run_data->t_start;
    run_data->rgb_fast = 0;
    run_data->rgb_fast_update = 0;
    run_data->switch_count = 0;
    run_data->lastAct = UNKNOWN;
    // run_data->count_down_init = 0;

    run_data->rgb_cfg.mode = 1;
    run_data->rgb_cfg.min_value_0 = 1;
    run_data->rgb_cfg.min_value_1 = 32;
    run_data->rgb_cfg.min_value_2 = 255;
    run_data->rgb_cfg.max_value_0 = 255;
    run_data->rgb_cfg.max_value_1 = 255;
    run_data->rgb_cfg.max_value_2 = 255;
    run_data->rgb_cfg.step_0 = 1;
    run_data->rgb_cfg.step_1 = 1;
    run_data->rgb_cfg.step_2 = 1;
    run_data->rgb_cfg.min_brightness = 0.15;
    run_data->rgb_cfg.max_brightness = 0.25;
    run_data->rgb_cfg.brightness_step = 0.003;
    run_data->rgb_cfg.time = 30;

    rgb_task_run(&run_data->rgb_cfg);
    sys->setSaverDisable(true); // 屏蔽屏保
    return 0;
}
static void time_switch()
{
    switch (run_data->time_mode)
    {
    case -1:
        run_data->time_start = millis();
        run_data->t_start.second = 0;
        run_data->t_start.minute = 5;
        run_data->t = run_data->t_start;
        run_data->rgb_fast = 0;
        run_data->rgb_fast_update = 0;
        break;
    case 0:
        run_data->time_start = millis();
        run_data->t_start.second = 0;
        run_data->t_start.minute = 25;
        run_data->t = run_data->t_start;
        run_data->rgb_fast = 0;
        run_data->rgb_fast_update = 0;
        break;
    case 1:
        run_data->time_start = millis();
        run_data->t_start.second = 0;
        run_data->t_start.minute = 45;
        run_data->t = run_data->t_start;
        run_data->rgb_fast = 0;
        run_data->rgb_fast_update = 0;
        break;
    case 2:
        run_data->time_start = millis();
        run_data->t_start.second = 0;
        run_data->t_start.minute = 15;
        run_data->t = run_data->t_start;
        run_data->rgb_fast = 0;
        run_data->rgb_fast_update = 0;
        break;

    default:
        break;
    }
}
/*********************************************************************************
 *Function:     rgb 控制
 *Description： 用来提醒
 *Calls:
 *Called By:
 *Input:
 *Output:
 *Return:
 *Others:       调快了速度和最低亮度
 **********************************************************************************/
static void rgb_ctrl()
{
    // Serial.print(run_data->rgb_fast);
    // Serial.println("     rgb_fast");
    // Serial.print(run_data->rgb_fast_update);
    // Serial.println("     rgb_fast_update");
    if (run_data->rgb_fast_update == 0)
    {
        if (run_data->rgb_fast == 1)
        {
            run_data->rgb_cfg.time = 10;
            // run_data->rgb_cfg.min_brightness = 0.01;
            // run_data->rgb_cfg.brightness_step = 0.05;
            // run_data->rgb_cfg.max_brightness = 0.95;
            // run_data->rgb_cfg.step_0 = 0;
            // run_data->rgb_cfg.step_1 = 0;
            // run_data->rgb_cfg.step_2 = 0;
            run_data->rgb_cfg.mode = 1;
            run_data->rgb_cfg.min_value_0 = 1;
            run_data->rgb_cfg.min_value_1 = 32;
            run_data->rgb_cfg.min_value_2 = 255;
            run_data->rgb_cfg.max_value_0 = 255;
            run_data->rgb_cfg.max_value_1 = 255;
            run_data->rgb_cfg.max_value_2 = 255;
            //  Serial.println("set low");
            run_data->rgb_cfg.step_0 = 1;
            run_data->rgb_cfg.step_1 = 1;
            run_data->rgb_cfg.step_2 = 1;
            run_data->rgb_cfg.min_brightness = 0.01;
            run_data->rgb_cfg.max_brightness = 1;
            run_data->rgb_cfg.brightness_step = 0.072;
            //  Serial.println("set fast");
        }
        else
        {
            run_data->rgb_cfg.mode = 1;
            run_data->rgb_cfg.min_value_0 = 1;
            run_data->rgb_cfg.min_value_1 = 32;
            run_data->rgb_cfg.min_value_2 = 255;
            run_data->rgb_cfg.max_value_0 = 255;
            run_data->rgb_cfg.max_value_1 = 255;
            run_data->rgb_cfg.max_value_2 = 255;
            //  Serial.println("set low");
            run_data->rgb_cfg.step_0 = 1;
            run_data->rgb_cfg.step_1 = 1;
            run_data->rgb_cfg.step_2 = 1;
            run_data->rgb_cfg.min_brightness = 0.15;
            run_data->rgb_cfg.max_brightness = 0.25;
            run_data->rgb_cfg.brightness_step = 0.001;
            run_data->rgb_cfg.time = 50;
        }
        rgb_task_run(&(run_data->rgb_cfg));
        run_data->rgb_fast_update = 1;
    }
}

static void tomato_process(AppController *sys, const ImuAction *act_info)
{
    if (!hadOpened)
    {
        delay(750);
        run_data->time_start = millis();
        hadOpened = true;
    }
    if (DOWN_MORE == act_info->active)
    {
        hadOpened = false;
        sys->app_exit(); // 退出APP
        return;
    }

    if (UP == act_info->active || DOWN == act_info->active)
    {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - run_data->time_start;
        int inc = UP == act_info->active ? 1 : -1;

        if (elapsedTime >= 300) // 控制时间增速
        {
            run_data->time_start = currentTime;
            if (run_data->t_start.minute < 99)
            {
                run_data->t_start.minute += inc;
            }
        }
        return;
    }

    if (TURN_LEFT == act_info->active || TURN_RIGHT == act_info->active)
    {
        static int last_mode;
        run_data->switch_count <<= 2;
        run_data->switch_count |= 3;      // 写11并移位
        if (run_data->switch_count > 0xf) // 只有连续的触发才进行切换
        {
            if (run_data->time_mode >= -1 && run_data->time_mode <= 2)

                if (run_data->lastAct != act_info->active)
                {
                    if (TURN_LEFT == act_info->active)
                    {
                        run_data->switch_count = 0X00;
                        run_data->time_mode -= 1;
                        if (run_data->time_mode >= -1 && run_data->time_mode <= 2)
                        {
                            run_data->t_start.minute = 5;
                            delay(50);
                            run_data->time_start = millis();
                        }
                    }
                    else if (TURN_RIGHT == act_info->active)
                    {
                        run_data->switch_count = 0X00;
                        run_data->time_mode += 1;
                        if (run_data->time_mode >= -1 && run_data->time_mode <= 2)
                        {
                            run_data->t_start.minute = 45;
                            delay(50);
                            run_data->time_start = millis();
                        }
                    }
                    if (run_data->time_mode > 2) // 限幅
                        run_data->time_mode = 2;
                    if (run_data->time_mode < -1)
                        run_data->time_mode = -1;
                    if (last_mode != run_data->time_mode)
                    {
                        time_switch(); // 发生模式切换时运行
                    }
                    last_mode = run_data->time_mode;
                }
        }
        run_data->lastAct = act_info->active;
    }
    else // 消抖 未触发，写00并移位
    {
        run_data->switch_count <<= 2;
        run_data->switch_count &= ~3;
    }

    if (millis() - run_data->time_start > 800)
    {
        run_data->lastAct = UNKNOWN;
    }

    if (run_data->t.minute == 0 && run_data->t.second == 0 && run_data->rgb_fast == 0) // 到点，rgb闪烁提醒
    {
        run_data->rgb_fast = 1;
        run_data->rgb_fast_update = 0;
    }
    rgb_ctrl();
    if (run_data->rgb_fast == 0) // 未到点持续计算之间
    {
        unsigned long ms_count = 999 + (run_data->t_start.second + run_data->t_start.minute * 60) * 1000; // 倒计时时长，单位ms，加999ms是为了可以显示x分00秒，否则会直接闪过
        run_data->time_ms = ms_count - (millis() - run_data->time_start);                                 // 倒计时长减去已经过去的时间就是要显示的时间
        run_data->t.second = run_data->time_ms % 60000 / 1000;
        run_data->t.minute = run_data->time_ms / 60 / 1000;
    }
    // Serial.print(run_data->rgb_fast);
    LVGL_OPERATE_LOCK(display_tomato(run_data->t, run_data->time_mode);)
    delay(100);
}

static int tomato_exit_callback(void *param)
{
//    rgb_task_run(&run_data->sys->rgb_cfg); // 重设rgb // app_controller.app_exit()会恢复rgb和屏保
    LVGL_OPERATE_LOCK(tomato_gui_del();)
    if (run_data != NULL)
    {
        // 释放资源
        free(run_data);
        run_data = NULL;
        Serial.println("EXIT\n");
    }
    return 0;
}
static void tomato_message_handle(const char *from, const char *to, APP_MESSAGE_TYPE type, void *message, void *ext_info)
{
    switch (type)
    {
    case APP_MESSAGE_WIFI_STA:
    {
        // get_timestamp(TIME_API);
    }
    }
}

APP_OBJ tomato_app = {TOMATO_APP_NAME, &app_tomato_icon, "Author Fjl\nVersion 1.0.0\n",
                      tomato_init, tomato_process,
                      tomato_exit_callback, tomato_message_handle};
