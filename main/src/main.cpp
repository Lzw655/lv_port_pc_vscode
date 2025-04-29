/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#if __has_include("esp_brookesia.hpp")
#include <mutex>
#include "esp_brookesia.hpp"
#define USE_ESP_BROOKESIA
#endif

/*********************
 *      DEFINES
 *********************/
#define DISP_HOR_RES  360
#define DISP_VER_RES  360

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_display_t * hal_init(int32_t w, int32_t h);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

extern void freertos_main(void);
extern int brookesia_speaker_init(int hor_res, int ver_res, esp_brookesia::gui::LockCallback lock_callback, esp_brookesia::gui::UnlockCallback unlock_callback);
extern void brookesia_speaker_init(int hor_res, int ver_res);

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

extern "C" int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  lv_init();

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  hal_init(DISP_HOR_RES, DISP_VER_RES);

  std::recursive_timed_mutex lock_mutex;

#if defined(USE_ESP_BROOKESIA)
# if ESP_BROOKESIA_CONF_SYSTEMS_ENABLE_PHONE
    brookesia_phone_init(DISP_HOR_RES, DISP_VER_RES);
# elif ESP_BROOKESIA_CONF_SYSTEMS_ENABLE_SPEAKER
    brookesia_speaker_init(DISP_HOR_RES, DISP_VER_RES, [&](int timeout_ms) {
        auto timeout = timeout_ms == 0 ? std::chrono::hours(24) : std::chrono::milliseconds(timeout_ms);
        if (lock_mutex.try_lock_for(timeout)) {
            return true;
        }
        return false;
    }, [&]() {
        lock_mutex.unlock();
    });
# endif // ESP_BROOKESIA_CONF_SYSTEMS_ENABLE_PHONE
#endif // USE_ESP_BROOKESIA

  #if LV_USE_OS == LV_OS_NONE

#if !defined(USE_ESP_BROOKESIA)
  lv_demo_widgets();
#endif

  while(1) {
    /* Periodically call the lv_task handler.
     * It could be done in a timer interrupt or an OS task too.*/
    lock_mutex.lock();
    lv_timer_handler();
    lock_mutex.unlock();
    usleep(5 * 1000);
  }

  #elif LV_USE_OS == LV_OS_FREERTOS

  /* Run FreeRTOS and create lvgl task */
  freertos_main();

  #endif

  return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static lv_display_t * hal_init(int32_t w, int32_t h)
{

  lv_group_set_default(lv_group_create());

  lv_display_t * disp = lv_sdl_window_create(w, h);

  lv_indev_t * mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, lv_group_get_default());
  lv_indev_set_display(mouse, disp);
  lv_display_set_default(disp);

  LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  lv_obj_t * cursor_obj;
  cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
  lv_image_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
  lv_indev_set_cursor(mouse, cursor_obj);             /*Connect the image  object to the driver*/

  lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_display(mousewheel, disp);
  lv_indev_set_group(mousewheel, lv_group_get_default());

  lv_indev_t * kb = lv_sdl_keyboard_create();
  lv_indev_set_display(kb, disp);
  lv_indev_set_group(kb, lv_group_get_default());

  return disp;
}
