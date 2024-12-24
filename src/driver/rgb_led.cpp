#include "rgb_led.h"
#include "common.h"
#include <Arduino.h>

void Pixel::init()
{
    FastLED.addLeds<WS2812, RGB_LED_PIN, GRB>(rgb_buffers, RGB_LED_NUM);
    FastLED.setBrightness(200);
}

Pixel &Pixel::setRGB(int r, int g, int b)
{
    for (int pos = 0; pos < RGB_LED_NUM; ++pos)
    {
        rgb_buffers[pos] = CRGB(r, g, b);
    }
    FastLED.show();

    return *this;
}

Pixel &Pixel::setHVS(uint8_t ih, uint8_t is, uint8_t iv)
{
    for (int pos = 0; pos < RGB_LED_NUM; ++pos)
    {
        rgb_buffers[pos].setHSV(ih, is, iv);
    }
    FastLED.show();

    return *this;
}

Pixel &Pixel::fill_rainbow(int min_r, int max_r,
                           int min_g, int max_g,
                           int min_b, int max_b)
{
    fill_gradient(rgb_buffers, 0, CHSV(50, 255, 255), 29, CHSV(150, 255, 255), SHORTEST_HUES);
    FastLED.show();

    return *this;
}

Pixel &Pixel::setBrightness(float duty)
{
    duty = constrain(duty, 0, 1);
    FastLED.setBrightness((uint8_t)(255 * duty));
    FastLED.show();

    return *this;
}

LED_RUN_MODE run_mode = RUN_MODE_NONE;
RgbParam g_rgb;
RgbRunStatus rgb_status;
BaseType_t taskRgbReturned;
TaskHandle_t handleLed = NULL;
TimerHandle_t xTimer_rgb = NULL;

void led_timerHandler(TimerHandle_t xTimer);
void led_taskHandler(void *parameter);
static void hsvModeChange(void);
static void rgbModeChange(void);
static void onceChange(void);
static void count_cur_brightness(void);

bool rgb_task_run(RgbParam *rgb_setting, LED_RUN_MODE mode)
{
    if (RUN_MODE_NONE <= mode)
    {
        // 创建失败
        return false;
    }

    if (run_mode != mode)
    {
        // 运行模式发生变化
        rgb_task_del();
        run_mode = mode;
    }

    g_rgb = *rgb_setting;

    // 拷贝数据
    if (LED_MODE_RGB == g_rgb.mode)
    {
        rgb_status.current_r = g_rgb.min_value_r;
        rgb_status.current_g = g_rgb.min_value_g;
        rgb_status.current_b = g_rgb.min_value_b;
        rgb_status.current_brightness = g_rgb.min_brightness;
        rgb_status.pos = 0;
    }
    else if (LED_MODE_HSV == g_rgb.mode)
    {
        rgb_status.current_h = g_rgb.min_value_h;
        rgb_status.current_s = g_rgb.min_value_s;
        rgb_status.current_v = g_rgb.min_value_v;
        rgb_status.current_brightness = g_rgb.min_brightness;
        rgb_status.pos = 0;
    }

    // 选择启动两种运行模式
    if (RUN_MODE_TIMER == run_mode)
    {
        if (NULL != xTimer_rgb)
        {
            xTimerStop(xTimer_rgb, 0);
            xTimer_rgb = NULL;
        }
        xTimer_rgb = xTimerCreate("led_timerHandler",
                                  g_rgb.time / portTICK_PERIOD_MS,
                                  pdTRUE, (void *)0, led_timerHandler);
        xTimerStart(xTimer_rgb, 0); // 开启定时器
    }
    else if (RUN_MODE_TASK == run_mode)
    {
        if (NULL == handleLed)
        {
            taskRgbReturned = xTaskCreate(
                led_taskHandler,
                "led_taskHandler",
                10 * 128, // 实际上 7*128就够用
                (void *)&g_rgb.time,
                TASK_RGB_PRIORITY,
                &handleLed);
            if (taskRgbReturned != pdPASS)
            {
                return false;
            }
        }
    }
    return true;
}

bool rgb_task_run(RgbConfig *rgb_cfg) {
    // 初始化RGB灯 HSV色彩模式
    RgbParam rgb_setting = {LED_MODE_HSV,
                            rgb_cfg->min_value_0, rgb_cfg->min_value_1, rgb_cfg->min_value_2,
                            rgb_cfg->max_value_0, rgb_cfg->max_value_1, rgb_cfg->max_value_2,
                            rgb_cfg->step_0, rgb_cfg->step_1, rgb_cfg->step_2,
                            rgb_cfg->min_brightness, rgb_cfg->max_brightness,
                            rgb_cfg->brightness_step, rgb_cfg->time};
    // 运行RGB任务
    return rgb_task_run(&rgb_setting, RUN_MODE_TASK);
}

void led_timerHandler(TimerHandle_t xTimer)
{
    onceChange();
}

void led_taskHandler(void *parameter)
{
    int *ms = (int *)parameter; // 控制时间
    for (;;)
    {
        onceChange();
        vTaskDelay(*ms);

        // if (pdTRUE == xSemaphoreTake(lvgl_mutex, portMAX_DELAY))
        // {
        //     lv_task_handler();
        //     xSemaphoreGive(lvgl_mutex);
        // }
    }
}

static void onceChange(void)
{
    if (LED_MODE_RGB == g_rgb.mode)
    {
        rgbModeChange();
    }
    else if (LED_MODE_HSV == g_rgb.mode)
    {
        hsvModeChange();
    }
}

static void hsvModeChange(void)
{
    // HSV色彩的控制
    rgb_status.current_h += g_rgb.step_h;
    if (rgb_status.current_h >= g_rgb.max_value_h)
    {
        g_rgb.step_h = (-1) * g_rgb.step_h;
        rgb_status.current_h = g_rgb.max_value_h;
    }
    else if (rgb_status.current_h <= g_rgb.min_value_h)
    {
        g_rgb.step_h = (-1) * g_rgb.step_h;
        rgb_status.current_h = g_rgb.min_value_h;
    }

    rgb_status.current_s += g_rgb.step_s;
    if (rgb_status.current_s >= g_rgb.max_value_s)
    {
        g_rgb.step_s = (-1) * g_rgb.step_s;
        rgb_status.current_s = g_rgb.max_value_s;
    }
    else if (rgb_status.current_s <= g_rgb.min_value_s)
    {
        g_rgb.step_s = (-1) * g_rgb.step_s;
        rgb_status.current_s = g_rgb.min_value_s;
    }

    rgb_status.current_v += g_rgb.step_v;
    if (rgb_status.current_v >= g_rgb.max_value_v)
    {
        g_rgb.step_v = (-1) * g_rgb.step_v;
        rgb_status.current_v = g_rgb.max_value_v;
    }
    else if (rgb_status.current_v <= g_rgb.min_value_v)
    {
        g_rgb.step_v = (-1) * g_rgb.step_v;
        rgb_status.current_v = g_rgb.min_value_v;
    }

    // 计算当前背光值
    count_cur_brightness();

    // 设置HSV状态
    rgb.setHVS(rgb_status.current_h,
               rgb_status.current_s,
               rgb_status.current_v)
        .setBrightness(rgb_status.current_brightness);
}

static void rgbModeChange(void)
{
    // RGB色彩的控制
    if (0 == rgb_status.pos) // 控制到R
    {
        rgb_status.current_r += g_rgb.step_r;
        if (rgb_status.current_r >= g_rgb.max_value_r && g_rgb.step_r > 0)
        {
            rgb_status.pos = 1;
            rgb_status.current_r = g_rgb.max_value_r;
        }
        else if (rgb_status.current_r <= g_rgb.min_value_r && g_rgb.step_r < 0)
        {
            g_rgb.step_r = (-1) * g_rgb.step_r;
            rgb_status.current_r = g_rgb.min_value_r;
        }
    }
    else if (1 == rgb_status.pos) // 控制到G
    {
        rgb_status.current_g += g_rgb.step_r;
        if (rgb_status.current_g >= g_rgb.max_value_g && g_rgb.step_r > 0)
        {
            rgb_status.pos = 2;
            rgb_status.current_g = g_rgb.max_value_g;
        }
        else if (rgb_status.current_g <= g_rgb.min_value_g && g_rgb.step_r < 0)
        {
            rgb_status.pos = 0;
            rgb_status.current_g = g_rgb.min_value_g;
        }
    }
    else if (2 == rgb_status.pos) // 控制到B
    {
        rgb_status.current_b += g_rgb.step_r;
        if (rgb_status.current_b >= g_rgb.max_value_b && g_rgb.step_r > 0)
        {
            g_rgb.step_r = (-1) * g_rgb.step_r;
            rgb_status.current_b = g_rgb.max_value_b;
        }
        else if (rgb_status.current_b <= g_rgb.min_value_b && g_rgb.step_r < 0)
        {
            rgb_status.pos = 1;
            rgb_status.current_b = g_rgb.min_value_b;
        }
    }

    // 计算当前背光值
    count_cur_brightness();

    // 设置RGB状态
    rgb.setRGB(rgb_status.current_r,
               rgb_status.current_g,
               rgb_status.current_b)
        .setBrightness(rgb_status.current_brightness);
}

static void count_cur_brightness(void)
{
    // 背光的控制
    rgb_status.current_brightness += g_rgb.brightness_step;
    if (rgb_status.current_brightness >= g_rgb.max_brightness)
    {
        rgb_status.current_brightness = g_rgb.max_brightness;
        g_rgb.brightness_step = (-1) * g_rgb.brightness_step;
    }
    else if (rgb_status.current_brightness <= g_rgb.min_brightness)
    {
        rgb_status.current_brightness = g_rgb.min_brightness;
        g_rgb.brightness_step = (-1) * g_rgb.brightness_step;
    }
}

void rgb_task_del(void)
{
    if (RUN_MODE_TIMER == run_mode && NULL != xTimer_rgb)
    {
        xTimerStop(xTimer_rgb, 0);
        xTimer_rgb = NULL;
    }
    if (RUN_MODE_TASK == run_mode && NULL != handleLed)
    {
        vTaskDelete(handleLed);
        handleLed = NULL;
    }
    rgb.setBrightness(0);
    run_mode = RUN_MODE_NONE;
}

void rgb_pause(void)
{
    // 暂停定时器
    if (RUN_MODE_TIMER == run_mode && NULL != xTimer_rgb)
    {
        xTimerStop(xTimer_rgb, 0);  // 停止定时器
    }
    // 暂停任务
    if (RUN_MODE_TASK == run_mode && NULL != handleLed)
    {
        vTaskSuspend(handleLed);  // 挂起任务
    }
    rgb.setBrightness(0);  // 设置背光为 0，暂停操作
}

void rgb_resume(void)
{
    // 恢复定时器
    if (RUN_MODE_TIMER == run_mode && NULL != xTimer_rgb)
    {
        xTimerStart(xTimer_rgb, 0);  // 启动定时器
    }
    // 恢复任务
    if (RUN_MODE_TASK == run_mode && NULL != handleLed)
    {
        vTaskResume(handleLed);  // 恢复任务
    }
}
