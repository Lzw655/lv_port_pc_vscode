/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include "esp_brookesia.hpp"
#include "private/esp_brookesia_utils.h"

/*********************
 *      DEFINES
 *********************/
#define EXAMPLE_PHONE_USE_STYLESHEET

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void get_local_time(struct tm *timeinfo, time_t *rawtime) {
#ifdef _WIN32
    // For Windows systems
    localtime_s(timeinfo, rawtime);
#else
    // For POSIX systems
    localtime_r(rawtime, timeinfo);
#endif
}
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
static void on_clock_update_timer_cb(struct _lv_timer_t *t);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int brookesia_phone_init(int hor_res, int ver_res)
{
    ESP_BROOKESIA_LOGI("Using display resolution: %dx%d", hor_res, ver_res);

    /* Create a phone object */
    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone();
    ESP_BROOKESIA_CHECK_NULL_RETURN(phone, 1, "Create phone failed");

#ifdef EXAMPLE_PHONE_USE_STYLESHEET
    /* Add external stylesheet and activate it */
    ESP_Brookesia_PhoneStylesheet_t *stylesheet = nullptr;
    if(hor_res == 320 && ver_res == 240) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_320_240_DARK_STYLESHEET();
    } else if (hor_res == 320 && ver_res == 480) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_320_480_DARK_STYLESHEET();
    } else if (hor_res == 480 && ver_res == 480) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_480_480_DARK_STYLESHEET();
    } else if (hor_res == 720 && ver_res == 1280) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_720_1280_DARK_STYLESHEET();
    } else if (hor_res == 800 && ver_res == 480) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_800_480_DARK_STYLESHEET();
    } else if (hor_res == 800 && ver_res == 1280) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_800_1280_DARK_STYLESHEET();
    } else if (hor_res == 1024 && ver_res == 600) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_1024_600_DARK_STYLESHEET();
    } else if (hor_res == 1280 && ver_res == 800) {
        stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_1280_800_DARK_STYLESHEET();
    } else {
        ESP_BROOKESIA_LOGE("Unsupported display resolution: %dx%d", hor_res, ver_res);
        goto next;
    }
    ESP_BROOKESIA_CHECK_NULL_RETURN(stylesheet, 1, "Create phone stylesheet failed");

    ESP_BROOKESIA_LOGI("Using stylesheet (%s)", stylesheet->core.name);
    ESP_BROOKESIA_CHECK_FALSE_RETURN(phone->addStylesheet(stylesheet), 1, "Add phone stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN(phone->activateStylesheet(stylesheet), 1, "Activate phone stylesheet failed");
    delete stylesheet;
#endif

next:
    /* Configure and begin the phone */
    ESP_BROOKESIA_CHECK_FALSE_RETURN(phone->begin(), 1, "Begin failed");
    // ESP_BROOKESIA_CHECK_FALSE_RETURN(phone->getCoreHome().showContainerBorder(), 1, "Show container border failed");

    /* Install apps */
    PhoneAppSimpleConf *app_simple_conf = new PhoneAppSimpleConf();
    ESP_BROOKESIA_CHECK_NULL_RETURN(app_simple_conf, 1, "Create app simple conf failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN((phone->installApp(app_simple_conf) >= 0), 1, "Install app simple conf failed");
    PhoneAppComplexConf *app_complex_conf = new PhoneAppComplexConf();
    ESP_BROOKESIA_CHECK_NULL_RETURN(app_complex_conf, 1, "Create app complex conf failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN((phone->installApp(app_complex_conf) >= 0), 1, "Install app complex conf failed");
    PhoneAppSquareline *app_squareline = PhoneAppSquareline::getInstance();
    ESP_BROOKESIA_CHECK_NULL_RETURN(app_squareline, 1, "Create app squareline failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN((phone->installApp(app_squareline) >= 0), 1, "Install app squareline failed");

    /* Create a timer to update the clock */
    ESP_BROOKESIA_CHECK_NULL_RETURN(lv_timer_create(on_clock_update_timer_cb, 1000, phone), 1, "Create clock update timer failed");

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void on_clock_update_timer_cb(struct _lv_timer_t *t)
{
    time_t now;
    struct tm timeinfo;
    bool is_time_pm = false;
    ESP_Brookesia_Phone *phone = (ESP_Brookesia_Phone *)t->user_data;

    time(&now);
    get_local_time(&timeinfo, &now);
    is_time_pm = (timeinfo.tm_hour >= 12);
    ESP_BROOKESIA_CHECK_FALSE_EXIT(
        phone->getHome().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min, is_time_pm),
        "Refresh status bar failed"
    );

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    uint32_t free_kb = mon.free_size / 1024;
    uint32_t total_kb = mon.total_size / 1024;
    ESP_BROOKESIA_CHECK_FALSE_EXIT(
        phone->getHome().getRecentsScreen()->setMemoryLabel(free_kb, total_kb, 0, 0),
        "Refresh memory label failed"
    );
}
