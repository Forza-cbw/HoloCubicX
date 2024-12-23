/***************************************************
  HoloCubic多功能固件源码
  （项目中若参考本工程源码，请注明参考来源）

  聚合多种APP，内置天气、时钟、相册、特效动画、视频播放、视频投影、
  浏览器文件修改。（各APP具体使用参考说明书）

  Github repositories：https://github.com/ClimbSnail/HoloCubic_AIO

  Last review/edit by ClimbSnail: 2023/01/14
 ****************************************************/
#include "driver/lv_port_indev.h"
#include "driver/lv_port_fs.h"

#include "common.h"
#include "gui_lock.h"
#include "sys/app_controller.h"

#include "app/app_conf.h"

#include <SPIFFS.h>
#include <esp32-hal.h>
#include <esp32-hal-timer.h>


bool isCheckAction = false;

/*** Component objects **7*/
ImuAction *act_info;           // 存放mpu6050返回的数据
AppController *app_controller; // APP控制器

TaskHandle_t handleTaskRefresh;

void refreshScreen(void *parameter)
{
    while (1)
    {
        LVGL_OPERATE_LOCK(lv_task_handler();) // 阻塞
        vTaskDelay(1000.0 / 60 / portTICK_PERIOD_MS);
    }
}


TimerHandle_t xTimerAction = NULL;
void actionCheckHandle(TimerHandle_t xTimer)
{
    // 标志需要检测动作
    isCheckAction = true;
}

void my_print(const char *buf)
{
    log_i("%s", buf);
    Serial.flush();
}

void setup()
{
    Serial.begin(115200);
    log_i("\nAIO (All in one) version " AIO_VERSION "\n");
    Serial.flush();
    // MAC ID可用作芯片唯一标识
    log_i("ChipID(EfuseMac): ");
    log_i("%lld",ESP.getEfuseMac());
    // flash运行模式
    // log_i(F("FlashChipMode: "));
    // log_i(ESP.getFlashChipMode());
    // log_i(F("FlashChipMode value: FM_QIO = 0, FM_QOUT = 1, FM_DIO = 2, FM_DOUT = 3, FM_FAST_READ = 4, FM_SLOW_READ = 5, FM_UNKNOWN = 255"));

    app_controller = new AppController(); // APP控制器

    // 需要放在Setup里初始化SPIFFS（闪存文件系统）
    if (!SPIFFS.begin(true))
    {
        log_i("SPIFFS Mount Failed");
        return;
    }

    // config_read(NULL, &g_cfg);   // 旧的配置文件读取方式
    app_controller->read_config(&app_controller->sys_cfg);
    app_controller->read_config(&app_controller->mpu_cfg);
    app_controller->read_config(&app_controller->rgb_cfg);

    /*** Init screen ***/
    screen.init(app_controller->sys_cfg.rotation,
                app_controller->sys_cfg.backLight);

    /*** Init on-board RGB ***/
    rgb.init();
    rgb.setBrightness(0.05).setRGB(0, 64, 64);


    /*** Init ambient-light sensor ***/
    ambLight.init(ONE_TIME_H_RESOLUTION_MODE);

    /*** Init micro SD-Card ***/
    tf.init();
    lv_fs_fatfs_init();

    // 自动刷新屏幕
     BaseType_t taskRefreshReturned =
             xTaskCreate(refreshScreen,
                         "refreshScreen", 8 * 1024,
                         nullptr, TASK_LVGL_PRIORITY, &handleTaskRefresh);
     if (taskRefreshReturned != pdPASS) log_e("taskRefreshReturned != pdPASS");
     else log_i("taskRefreshReturned == pdPASS");


#if LV_USE_LOG
    lv_log_register_print_cb(my_print);
#endif /*LV_USE_LOG*/

    app_controller->init();

// 将APP"安装"到controller里
#if APP_WEATHER_USE
    app_controller->app_install(&weather_app);
#endif
#if APP_TOMATO_USE
    app_controller->app_install(&tomato_app);
#endif
#if APP_PICTURE_USE
    app_controller->app_install(&picture_app);
#endif
#if APP_MEDIA_PLAYER_USE
    app_controller->app_install(&media_app);
#endif
#if APP_FILE_MANAGER_USE
    app_controller->app_install(&file_manager_app);
#endif
#if APP_WEB_SERVER_USE
    app_controller->app_install(&server_app);
#endif
#if APP_IDEA_ANIM_USE
    app_controller->app_install(&idea_app);
#endif
#if APP_SETTING_USE
    app_controller->app_install(&settings_app);
#endif
#if APP_GAME_2048_USE
    app_controller->app_install(&game_2048_app);
#endif
#if APP_GAME_SNAKE_USE
    app_controller->app_install(&game_snake_app);
#endif
#if APP_ANNIVERSARY_USE
    app_controller->app_install(&anniversary_app);
#endif
#if APP_ARCHER_USE
    app_controller->app_install(&archer_app);
#endif
#if APP_PC_RESOURCE_USE
    app_controller->app_install(&pc_resource_app);
#endif
#if APP_LHLXW_USE
    app_controller->app_install(&LHLXW_app);
#endif

    // 优先显示屏幕 加快视觉上的开机时间
    app_controller->main_process(&mpu.action_info);

    /*** Init IMU as input device ***/
    // lv_port_indev_init();

    mpu.init(app_controller->sys_cfg.mpu_order,
             app_controller->sys_cfg.auto_calibration_mpu,
             &app_controller->mpu_cfg); // 初始化比较耗时

    /*** 以此作为MPU6050初始化完成的标志 ***/
    RgbConfig *rgb_cfg = &app_controller->rgb_cfg;
    // 初始化RGB灯 HSV色彩模式
    RgbParam rgb_setting = {LED_MODE_HSV,
                            rgb_cfg->min_value_0, rgb_cfg->min_value_1, rgb_cfg->min_value_2,
                            rgb_cfg->max_value_0, rgb_cfg->max_value_1, rgb_cfg->max_value_2,
                            rgb_cfg->step_0, rgb_cfg->step_1, rgb_cfg->step_2,
                            rgb_cfg->min_brightness, rgb_cfg->max_brightness,
                            rgb_cfg->brightness_step, rgb_cfg->time};
    // 运行RGB任务
    set_rgb_and_run(&rgb_setting, RUN_MODE_TASK);

    // 先初始化一次动作数据 防空指针
    act_info = mpu.getAction();
    // 定义一个mpu6050的动作检测定时器（每200ms将isCheckAction置为true）
    xTimerAction = xTimerCreate("Action Check",
                                200 / portTICK_PERIOD_MS,
                                pdTRUE, (void *)0, actionCheckHandle);
    xTimerStart(xTimerAction, 0);

    // 自启动APP
    app_controller->app_auto_start();
}

// 在cores/esp32/main.cpp 中启动为FreeRTOS task，优先级为1（最低）。
// FreeRTOSConfig.h中宏configUSE_PREEMPTION为1，是抢占式调度器。同优先级时间片轮转，高优先级抢占。
// ESP32-s3 双核
void loop()
{
//    screen.routine(); // 手动刷新屏幕

    if (isCheckAction)
    {
        isCheckAction = false;
        act_info = mpu.getAction(); // 更新姿态
    }
    app_controller->main_process(act_info); // 运行当前进程
    // log_i(ambLight.getLux() / 50.0);
    // rgb.setBrightness(ambLight.getLux() / 500.0);
}