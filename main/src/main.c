
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */

#include <stdlib.h>
#include <unistd.h>

#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

#include "SDL2/SDL.h"
#include "lvgl/lvgl.h"


#include "monitor.h"
#include "mouse.h"
#include "keyboard.h"
#include "mousewheel.h"

#include "generated/gui_guider.h"
#include "custom/custom.h"
#include "generated/events_init.h"


//#include <stdio.h>
//#include <Python.h>


lv_ui guider_ui;

static void hal_init(void);

static int tick_thread(void *data);

int main(int argc, char **argv) {
    (void) argc; /*Unused*/
    (void) argv; /*Unused*/

    /*Initialize LVGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init();

//     Py_Initialize(); // 初始化Python解释器

//     if (Py_IsInitialized()) {
//         // 创建一个Python文件对象
//         PyObject *file = PyFile_FromFile(argv[1], "r", NULL, NULL);
//         if (file == NULL) {
//             fprintf(stderr, "Failed to open file\n");
//             goto error;
//         }

//         // 执行Python文件
//         PyObject *result = PyRun_File(file, argv[1], Py_file_input, NULL, NULL);
//         if (result == NULL) {
//             fprintf(stderr, "Error executing file\n");
//             goto error;
//         }
//         Py_DECREF(result);

//         Py_DECREF(file); // 释放文件对象
//     } else {
//         fprintf(stderr, "Python initialization failed\n");
//         goto error;
//     }

//     Py_Finalize(); // 清理Python解释器
//     return 0;

// error:
//     Py_Finalize();
//     return 1;

    textarea_init();

    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);

    while (1) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        //lv_timer_handler();
        usleep(lv_timer_handler() * 1000);
    }

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void hal_init(void) {
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    monitor_init();
    /* Tick init.
     * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about
     * how much time were elapsed Create an SDL thread to do this*/
    SDL_CreateThread(tick_thread, "tick", NULL);

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[MONITOR_HOR_RES * 100];
    static lv_color_t buf1_2[MONITOR_HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, buf1_2, MONITOR_HOR_RES * 100);
    
    
    
    
    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = monitor_flush;
    disp_drv.hor_res = MONITOR_HOR_RES;
    disp_drv.ver_res = MONITOR_VER_RES;
    disp_drv.antialiasing = 1;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    mouse_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = mouse_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

    keyboard_init();
    static lv_indev_drv_t indev_drv_2;
    lv_indev_drv_init(&indev_drv_2); /*Basic initialization*/
    indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_2.read_cb = keyboard_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_2);
    lv_indev_set_group(kb_indev, g);
    mousewheel_init();
    static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = mousewheel_read;

    lv_indev_t *enc_indev = lv_indev_drv_register(&indev_drv_3);
    lv_indev_set_group(enc_indev, g);

    /*Set a cursor for the mouse*/
#if LV_USE_IMG != 0
    LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
    lv_obj_t *cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/
#endif
}

/**
 * A task to measure the elapsed time for LVGL
 * @param data unused
 * @return never return
 */
static int tick_thread(void *data) {
    (void) data;

    while (1) {
        SDL_Delay(5);
        lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
    }

    return 0;
}
