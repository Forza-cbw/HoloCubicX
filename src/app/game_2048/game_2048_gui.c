#include "game_2048_gui.h"
#include "element_images.h"

#include "lvgl.h"
#include "esp32-hal-log.h"
#include "gui_lock.h"

#define SCALE_SIZE 4

lv_obj_t *game_2048_gui = NULL;
lv_obj_t *img[SCALE_SIZE * SCALE_SIZE];

static lv_style_t default_style;

void game_2048_gui_init(void)
{
    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style, lv_color_hex(0x000000));

    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == game_2048_gui)
        return;
    lv_obj_clean(act_obj); // 清空此前页面

    //创建屏幕对象
    game_2048_gui = lv_obj_create(NULL);
    lv_obj_add_style(game_2048_gui, &default_style, LV_STATE_DEFAULT);

    for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
    {
        img[i] = lv_img_create(game_2048_gui);
        lv_img_set_src(img[i], &N0);
        lv_obj_align(img[i], LV_ALIGN_TOP_LEFT, 8 + i % 4 * 58, 8 + i / 4 * 58);
    }
    lv_scr_load(game_2048_gui);
}

void game_2048_gui_del(void)
{
    if (NULL != game_2048_gui)
    {
        lv_obj_clean(game_2048_gui);
        game_2048_gui = NULL;
    }

    // 手动清除样式，防止内存泄漏
     lv_style_reset(&default_style);
}

//用于宽高同时增加的lv_anim_exec_xcb_t动画参数
static void anim_size_cb(void *var, int32_t v)
{
    lv_obj_set_size(var, v, v);
}

//用于斜向移动的lv_anim_exec_xcb_t动画参数
static void anim_pos_cb(void *var, int32_t v)
{
    lv_obj_set_pos(var, v, v);
}

/*
 * 出生动画
 * i：出生的位置
 */
void render_born(int i)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_size_cb);
    lv_anim_set_var(&a, img[i]);
    lv_anim_set_time(&a, 200);

    /* 在动画中设置路径 */
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_values(&a, 0, 50);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_values(&a, lv_obj_get_x(img[i]) + 25, lv_obj_get_x(img[i]));
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(img[i]) + 25, lv_obj_get_y(img[i]));
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_x);
    lv_anim_set_values(&a, -25, 0);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_y);
    lv_anim_set_values(&a, -25, 0);
    lv_anim_start(&a);
}

/*
 * 合并动画
 * i：合并的位置
 */
void render_zoom(int i)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_size_cb);
    lv_anim_set_var(&a, img[i]);
    lv_anim_set_delay(&a, 0);
    lv_anim_set_time(&a, 100);
    //播完后回放
    lv_anim_set_playback_delay(&a, 0);
    lv_anim_set_playback_time(&a, 100);

    //线性动画
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_values(&a, 50, 56);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_values(&a, lv_obj_get_x(img[i]), lv_obj_get_x(img[i]) - 3);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(img[i]), lv_obj_get_y(img[i]) - 3);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_x);
    lv_anim_set_values(&a, 0, 3);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_offset_y);
    lv_anim_set_values(&a, 0, 3);
    lv_anim_start(&a);

    log_i("(%d,%d)", i/SCALE_SIZE, i%SCALE_SIZE);
}

/*
 * 移动动画
 * i：移动的目标对象
 * direction：移动的方向，lv_obj_set_x或lv_obj_set_y
 * dist：移动的距离，如1、-1
 */
void render_move(int i, lv_anim_exec_xcb_t direction, int dist)
{
    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)direction);
    lv_anim_set_var(&a, img[i]);
    lv_anim_set_time(&a, 500);
    if (direction == (lv_anim_exec_xcb_t)lv_obj_set_x)
    {
        lv_anim_set_values(&a, lv_obj_get_x(img[i]), lv_obj_get_x(img[i]) + dist * 58);
    }
    else
    {
        lv_anim_set_values(&a, lv_obj_get_y(img[i]), lv_obj_get_y(img[i]) + dist * 58);
    }

    // 在动画中设置路径
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);
}

//获取图片内容对象
const lv_img_dsc_t *getN(int i)
{
    switch (i)
    {
    case 0:
        return &N0;
    case 2:
        return &N2;
    case 4:
        return &N4;
    case 8:
        return &N8;
    case 16:
        return &N16;
    case 32:
        return &N32;
    case 64:
        return &N64;
    case 128:
        return &N128;
    case 256:
        return &N256;
    case 512:
        return &N512;
    case 1024:
        return &N1024;
    case 2048:
        return &N2048;
    default:
        return &N0;
    }
}

//刷新棋盘
void render_board(int *map)
{
    for (int i = 0; i < SCALE_SIZE * SCALE_SIZE; i++)
    {
        lv_img_set_src(img[i], getN(map[i])); // 设置图片像素
        lv_obj_align(img[i], LV_ALIGN_TOP_LEFT, 8 + i % 4 * 58, 8 + i / 4 * 58); // 设置图片位置
    }
}

/*
 * showAnim————用动画来更新棋盘
 * moveRecord：向x或y方向上移动多少格
 * direction：移动的方向，1.上 2.下 3.左 4.右
 */
void showAnim(int (*moveRecord)[4], bool (*dstNeedZoom)[4], int direction, int* board)
{
    assert(1 <= direction && direction <= 4);
    lv_anim_exec_xcb_t Normal = direction <= 2 ? (lv_anim_exec_xcb_t)lv_obj_set_y : (lv_anim_exec_xcb_t)lv_obj_set_x;

    int startI = 0, endI = SCALE_SIZE, stepI = 1;
    int startJ = 0, endJ = SCALE_SIZE, stepJ = 1;

    if (direction == 2) // 下
        startI = SCALE_SIZE-1, endI = -1, stepI = -1;
    if (direction == 4) // 右
        startJ = SCALE_SIZE-1, endJ = -1, stepJ = -1;

    //移动
    for (int i = startI; i != endI; i += stepI) {
        for (int j = startJ; j != endJ; j += stepJ) {
            int img_index = i * SCALE_SIZE + j; // 被移动img的下标

            if (moveRecord[i][j] != 0) {
                LVGL_OPERATE_LOCK(render_move(img_index, Normal, moveRecord[i][j]);)
            }
        }
    }
    waitForAinm(); // 等待动画完成，因为接下来的缓冲区同步会导致动画被打断
    // 合并
    for (int i = startI; i != endI; i += stepI) {
        for (int j = startJ; j != endJ; j += stepJ) {
            // 移动的目标位置
            int dst_i = i, dst_j = j;
            if (direction <= 2) dst_i += moveRecord[i][j];
            else dst_j += moveRecord[i][j];
            LVGL_OPERATE_LOCK(render_board(board));
            if (dstNeedZoom[i][j])
                LVGL_OPERATE_LOCK(render_zoom(dst_i * SCALE_SIZE + dst_j);)
        }
    }
    waitForAinm();
}

/*
 * newborn：新棋子的位置
 * map：新棋盘的地址
 */
void showNewBorn(int newborn, int *map)
{
    //GUI缓冲区同步
    LVGL_OPERATE_LOCK(render_board(map);)
    for (int i=0; i<4; i++)
        log_i("%d %d %d %d", map[i*4], map[i*4 + 1], map[i*4 +2], map[i*4 +3]);
    //出现
    LVGL_OPERATE_LOCK(render_born(newborn);)
    waitForAinm();
}