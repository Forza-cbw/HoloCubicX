#include "weather_gui.h"
#include "weather_image.h"
#include "Arduino.h"

#include "lvgl.h"

LV_FONT_DECLARE(lv_font_ibmplex_115);
LV_FONT_DECLARE(lv_font_ibmplex_64);
LV_FONT_DECLARE(ch_font20);
static lv_style_t default_style;
static lv_style_t chFont_style;
static lv_style_t numberSmall_style;
static lv_style_t numberBig_style;
static lv_style_t btn_style;
static lv_style_t bar_style;

static lv_obj_t *scr_1 = NULL;

static lv_obj_t *weatherImg = NULL;
static lv_obj_t *cityLabel = NULL;
static lv_obj_t *btn = NULL, *btnLabel = NULL;
static lv_obj_t *txtLabel = NULL;
static lv_obj_t *clockLabel_1 = NULL, *clockLabel_2 = NULL;
static lv_obj_t *dateLabel = NULL;
static lv_obj_t *tempImg = NULL, *tempBar = NULL, *tempLabel = NULL;
static lv_obj_t *humiImg = NULL, *humiBar = NULL, *humiLabel = NULL;
static lv_obj_t *spaceImg = NULL;


// 天气图标路径的映射关系
const void *weaImage_map[] = {&weather_0, &weather_9, &weather_14, &weather_5, &weather_25,
                              &weather_30, &weather_26, &weather_11, &weather_23};
// 太空人图标路径的映射关系
const void *manImage_map[] = {&man_0, &man_1, &man_2, &man_3, &man_4, &man_5, &man_6, &man_7, &man_8, &man_9};
static const char weekDayCh[7][4] = {"日", "一", "二", "三", "四", "五", "六"};

void weather_gui_init(void)
{
    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style, lv_color_hex(0x000000));

    lv_style_init(&chFont_style);
    lv_style_set_text_opa(&chFont_style, LV_OPA_COVER);
    lv_style_set_text_color(&chFont_style, lv_color_hex(0xffffff));
    lv_style_set_text_font(&chFont_style, &ch_font20);

    lv_style_init(&numberSmall_style);
    lv_style_set_text_opa(&numberSmall_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberSmall_style, lv_color_hex(0xffffff));
    lv_style_set_text_font(&numberSmall_style, &lv_font_ibmplex_64);

    lv_style_init(&numberBig_style);
    lv_style_set_text_opa(&numberBig_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberBig_style, lv_color_hex(0xffffff));
    lv_style_set_text_font(&numberBig_style, &lv_font_ibmplex_115);

    lv_style_init(&btn_style);
    lv_style_set_border_width(&btn_style, 0);

    lv_style_init(&bar_style);
    lv_style_set_bg_color(&bar_style, lv_color_hex(0x000000));
    lv_style_set_border_width(&bar_style, 2);
    lv_style_set_border_color(&bar_style, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_top(&bar_style, 1); // 指示器到背景四周的距离
    lv_style_set_pad_bottom(&bar_style, 1);
    lv_style_set_pad_left(&bar_style, 1);
    lv_style_set_pad_right(&bar_style, 1);
}

void display_weather_init(void)
{
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == scr_1)
        return;

    weather_gui_release();
    lv_obj_clean(act_obj); // 清空此前页面

    scr_1 = lv_obj_create(NULL);
    lv_obj_add_style(scr_1, &default_style, LV_STATE_DEFAULT);

    weatherImg = lv_img_create(scr_1);
    lv_img_set_src(weatherImg, weaImage_map[0]);

    cityLabel = lv_label_create(scr_1);
    lv_obj_add_style(cityLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_label_set_recolor(cityLabel, true);
    lv_label_set_text(cityLabel, "上海");

    btn = lv_btn_create(scr_1);
    lv_obj_add_style(btn, &btn_style, LV_STATE_DEFAULT);
    lv_obj_set_pos(btn, 75, 15);
    lv_obj_set_size(btn, 50, 25);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_DEFAULT);

    btnLabel = lv_label_create(btn);
    lv_obj_add_style(btnLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_obj_align(btnLabel, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(btnLabel, "--");

    txtLabel = lv_label_create(scr_1);
    lv_obj_add_style(txtLabel, &chFont_style, LV_STATE_DEFAULT);
    // lvgl8之前版本，模式一旦设置 LV_LABEL_LONG_SCROLL_CIRCULAR
    // 宽度恒定等于当前文本的长度，所以下面先设置以下长度
    lv_label_set_text(txtLabel, "最低气温12°C, ");
    lv_obj_set_size(txtLabel, 120, 30);
    lv_label_set_long_mode(txtLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text_fmt(txtLabel, "晴天, %s 0 级.   ", "风力");
    lv_obj_set_style_anim_speed(txtLabel, 40, 0); // 控制滚动速度


    clockLabel_1 = lv_label_create(scr_1);
    lv_obj_add_style(clockLabel_1, &numberBig_style, LV_STATE_DEFAULT);
    lv_label_set_recolor(clockLabel_1, true);
    lv_label_set_text_fmt(clockLabel_1, "%02d#ffa500 %02d#", 10, 52);

    clockLabel_2 = lv_label_create(scr_1);
    lv_obj_add_style(clockLabel_2, &numberSmall_style, LV_STATE_DEFAULT);
    lv_label_set_recolor(clockLabel_2, true);
    lv_label_set_text_fmt(clockLabel_2, "%02d", 00);

    dateLabel = lv_label_create(scr_1);
    lv_obj_add_style(dateLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_label_set_text_fmt(dateLabel, "%2d月%2d日   周%s", 11, 23, weekDayCh[1]);

    tempImg = lv_img_create(scr_1);
    lv_img_set_src(tempImg, &temp);
    lv_img_set_zoom(tempImg, 180);
    tempBar = lv_bar_create(scr_1);
    lv_obj_add_style(tempBar, &bar_style, LV_STATE_DEFAULT);
    lv_bar_set_range(tempBar, -10, 45); // 设置进度条表示的温度为-10~45
    lv_obj_set_size(tempBar, 60, 12);
    lv_obj_set_style_bg_color(tempBar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_bar_set_value(tempBar, 10, LV_ANIM_ON);
    tempLabel = lv_label_create(scr_1);
    lv_obj_add_style(tempLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_label_set_text_fmt(tempLabel, "%2d°C", 18);

    humiImg = lv_img_create(scr_1);
    lv_img_set_src(humiImg, &humi);
    lv_img_set_zoom(humiImg, 180);
    humiBar = lv_bar_create(scr_1);
    lv_obj_add_style(humiBar, &bar_style, LV_STATE_DEFAULT);
    lv_bar_set_range(humiBar, 0, 100);
    lv_obj_set_size(humiBar, 60, 12);
    lv_obj_set_style_bg_color(humiBar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_bar_set_value(humiBar, 49, LV_ANIM_ON);
    humiLabel = lv_label_create(scr_1);
    lv_obj_add_style(humiLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_label_set_text(humiLabel, "50%");

    // 太空人图标
    spaceImg = lv_img_create(scr_1);
    lv_img_set_src(spaceImg, manImage_map[0]);

    // 绘制图形
    lv_obj_align(weatherImg, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_align(cityLabel, LV_ALIGN_TOP_LEFT, 20, 25);
    lv_obj_align(txtLabel, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_align(tempImg, LV_ALIGN_LEFT_MID, 10, 70);
    lv_obj_align(tempBar, LV_ALIGN_LEFT_MID, 35, 70);
    lv_obj_align(tempLabel, LV_ALIGN_LEFT_MID, 103, 70);
    lv_obj_align(humiImg, LV_ALIGN_LEFT_MID, 0, 100);
    lv_obj_align(humiBar, LV_ALIGN_LEFT_MID, 35, 100);
    lv_obj_align(humiLabel, LV_ALIGN_LEFT_MID, 103, 100);
    lv_obj_align(spaceImg, LV_ALIGN_BOTTOM_RIGHT, 0, -10);

    lv_obj_align(clockLabel_1, LV_ALIGN_LEFT_MID, 0, 10);
    lv_obj_align(clockLabel_2, LV_ALIGN_LEFT_MID, 165, 9);
    lv_obj_align(dateLabel, LV_ALIGN_LEFT_MID, 10, 38);
}

void render_state(int timeStatus, int weatherStatus) {
    char str[10] = "TW";

    if (WEATHER_STATUS_UPDATING == timeStatus || WEATHER_STATUS_UPDATING == weatherStatus) {
        strcpy(str, "更新");
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    }
    else if (WEATHER_STATUS_EXPIRED == timeStatus || WEATHER_STATUS_EXPIRED == weatherStatus) {
        if (WEATHER_STATUS_EXPIRED == timeStatus) str[0] = '-';
        if (WEATHER_STATUS_EXPIRED == weatherStatus) str[1] = '-';
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_DEFAULT);
    }
    else {
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), LV_STATE_DEFAULT);
    }

    lv_label_set_text(btnLabel, str);
}

void render_weather(struct Weather weaInfo)
{
    lv_label_set_text(cityLabel, weaInfo.cityname);
    if (strlen(weaInfo.cityname) > 6)
    {
        lv_obj_align(cityLabel, LV_ALIGN_TOP_LEFT, 5, 15);
    }
    else
    {
        lv_obj_align(cityLabel, LV_ALIGN_TOP_LEFT, 20, 15);
    }

    lv_img_set_src(weatherImg, weaImage_map[weaInfo.weather_code]);
    // 下面这行代码可能会出错
    lv_label_set_text_fmt(txtLabel, "   今日天气:%s,%s风%s 级.        ",
                          weaInfo.weather, weaInfo.windDir, weaInfo.windpower);

    lv_bar_set_value(tempBar, weaInfo.temperature, LV_ANIM_ON);
    lv_label_set_text_fmt(tempLabel, "%2d°C", weaInfo.temperature);
    lv_bar_set_value(humiBar, weaInfo.humidity, LV_ANIM_ON);
    lv_label_set_text_fmt(humiLabel, "%d%%", weaInfo.humidity);

    lv_scr_load(scr_1);
}

void render_time(struct TimeStr timeInfo)
{
    lv_label_set_text_fmt(clockLabel_1, "%02d#ffa500 %02d#", timeInfo.hour, timeInfo.minute);
    lv_label_set_text_fmt(clockLabel_2, "%02d", timeInfo.second);
    lv_label_set_text_fmt(dateLabel, "%2d月%2d日   周%s", timeInfo.month, timeInfo.day,
                          weekDayCh[timeInfo.weekday]);
}


void render_man(void)
{
    static int _spaceIndex = 0;
    if (NULL != scr_1 && lv_scr_act() == scr_1)
    {
        lv_img_set_src(spaceImg, manImage_map[_spaceIndex]);
        _spaceIndex = (_spaceIndex + 1) % 10;
    }
}


void weather_gui_release(void)
{
    if (scr_1 != NULL)
    {
        lv_obj_clean(scr_1);
        scr_1 = NULL;
        weatherImg = NULL;
        cityLabel = NULL;
        btn = NULL;
        btnLabel = NULL;
        txtLabel = NULL;
        clockLabel_1 = NULL;
        clockLabel_2 = NULL;
        dateLabel = NULL;
        tempImg = NULL;
        tempBar = NULL;
        tempLabel = NULL;
        humiImg = NULL;
        humiBar = NULL;
        humiLabel = NULL;
        spaceImg = NULL;
    }
}

void weather_gui_del(void)
{
    weather_gui_release();

    // 手动清除样式，防止内存泄漏
     lv_style_reset(&default_style);
     lv_style_reset(&chFont_style);
     lv_style_reset(&numberSmall_style);
     lv_style_reset(&numberBig_style);
     lv_style_reset(&btn_style);
     lv_style_reset(&bar_style);
}

int my_isdigit(char c) {
    return (c >= '0' && c <= '9');
}


int extractNumbers(const char *str) {
    int result = 0;
    int i;
    int len = strlen(str);
    for (i = 0; i < len; i++) {
        if (my_isdigit(str[i])) {
            result = result * 10 + (str[i] - '0');
        }
    }
    return result;
}
