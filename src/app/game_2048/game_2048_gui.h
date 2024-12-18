#ifndef APP_GAME_2048_GUI_H
#define APP_GAME_2048_GUI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

#define ANIEND_WAIT                 \
    while (lv_anim_count_running()) \
        lv_task_handler(); // 等待动画完成

    void game_2048_gui_init(void);
    void game_2048_gui_del(void);

	const lv_img_dsc_t* getN(int i);
	void render_board(int* map);
    void showAnim(int (*moveRecord)[4], bool (*dstNeedZoom)[4], int direction, int* board);
    void showNewBorn(int newborn, int *map);
	void render_born(int i);

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    extern const lv_img_dsc_t app_game_2048;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif