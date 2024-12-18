#ifndef APP_HEARTBEAT_GUI_H
#define APP_HEARTBEAT_GUI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
enum S_R_TYPE{
    SEND = 0,
    RECV,
    HEART,
};
#define ANIEND                      \
    while (lv_anim_count_running()) \
        lv_task_handler(); //等待动画完成

    void archer_gui_init(void);
    void display_archer(void);
    void archer_gui_del(void);
    void display_archer_img(void);
    void archer_set_sr_type(enum S_R_TYPE type);
    void archer_set_send_recv_cnt_label(uint8_t send_num, uint8_t recv_num);

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    extern const lv_img_dsc_t app_archer;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif