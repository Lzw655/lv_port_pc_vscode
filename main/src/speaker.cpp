/*********************
 *      INCLUDES
 *********************/
#define BOOST_THREAD_VERSION 4
#include <assert.h>
#include <cstdio>
#include "esp_brookesia.hpp"
#include "boost/thread.hpp"
#include "boost/signals2.hpp"
#include "boost/thread/pthread/thread_data.hpp"
#include "esp_brookesia_apps_speaker_settings.hpp"
#include "private/esp_brookesia_utils.h"
#include "systems/core/esp_brookesia_core.hpp"
#include "systems/speaker/widgets/quick_settings/esp_brookesia_speaker_quick_settings.hpp"

using namespace esp_brookesia::systems::speaker;
using namespace esp_brookesia::apps::speaker;

/*********************
 *      DEFINES
 *********************/
#define EXAMPLE_SPEAKER_USE_STYLESHEET

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

int brookesia_speaker_init(int hor_res, int ver_res, esp_brookesia::gui::LockCallback lock_callback, esp_brookesia::gui::UnlockCallback unlock_callback)
{
    ESP_BROOKESIA_LOGI("Using display resolution: %dx%d", hor_res, ver_res);

    /* Create a speaker object */
    Speaker *speaker = new Speaker();
    ESP_BROOKESIA_CHECK_NULL_RETURN(speaker, 1, "Create speaker failed");

#ifdef EXAMPLE_SPEAKER_USE_STYLESHEET
    /* Add external stylesheet and activate it */
    SpeakerStylesheet_t *stylesheet = nullptr;
    if(hor_res == 240 && ver_res == 240) {
        stylesheet = new SpeakerStylesheet_t(ESP_BROOKESIA_SPEAKER_240_240_DARK_STYLESHEET);
    } else if (hor_res == 360 && ver_res == 360) {
        stylesheet = new SpeakerStylesheet_t(ESP_BROOKESIA_SPEAKER_360_360_DARK_STYLESHEET);
    } else {
        ESP_BROOKESIA_LOGE("Unsupported display resolution: %dx%d", hor_res, ver_res);
        goto next;
    }
    ESP_BROOKESIA_CHECK_NULL_RETURN(stylesheet, 1, "Create speaker stylesheet failed");

    ESP_BROOKESIA_LOGI("Using stylesheet (%s)", stylesheet->core.name);
    stylesheet->core.screen_size.flags.enable_circle = true;
    stylesheet->display.ai_face.data.refresh.interval_ms = 20;
    stylesheet->display.ai_face.data.refresh.update_count = 1;
    ESP_BROOKESIA_CHECK_FALSE_RETURN(speaker->addStylesheet(stylesheet), 1, "Add speaker stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN(speaker->activateStylesheet(stylesheet), 1, "Activate speaker stylesheet failed");
    delete stylesheet;
#endif

next:
    /* Configure and begin the speaker */
    speaker->registerLvLockCallback(lock_callback, 0);
    speaker->registerLvUnlockCallback(unlock_callback);
    ESP_BROOKESIA_CHECK_FALSE_RETURN(speaker->begin(), 1, "Begin failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN(speaker->getCoreHome().showContainerBorder(), 1, "Show container border failed");

    /* Install apps */
    AppSimpleConf *app_simple_conf = new AppSimpleConf();
    ESP_BROOKESIA_CHECK_NULL_RETURN(app_simple_conf, 1, "Create app simple conf failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN((speaker->installApp(app_simple_conf) >= 0), 1, "Install app simple conf failed");
    // AppComplexConf *app_complex_conf = new AppComplexConf();
    // ESP_BROOKESIA_CHECK_NULL_RETURN(app_complex_conf, 1, "Create app complex conf failed");
    // ESP_BROOKESIA_CHECK_FALSE_RETURN((speaker->installApp(app_complex_conf) >= 0), 1, "Install app complex conf failed");
    // SpeakerAppSquareline *app_squareline = SpeakerAppSquareline::getInstance();
    // ESP_BROOKESIA_CHECK_NULL_RETURN(app_squareline, 1, "Create app squareline failed");
    // ESP_BROOKESIA_CHECK_FALSE_RETURN((speaker->installApp(app_squareline) >= 0), 1, "Install app squareline failed");

    Settings *app_settings = new Settings();
    ESP_BROOKESIA_CHECK_NULL_RETURN(app_settings, false, "Create app settings failed");
    SettingsStylesheetData *app_settings_stylesheet = new SettingsStylesheetData(SETTINGS_UI_360_360_STYLESHEET_DARK());
    ESP_BROOKESIA_CHECK_NULL_RETURN(app_settings_stylesheet, false, "Create app settings stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_RETURN(
        app_settings->addStylesheet(speaker, app_settings_stylesheet), false, "Add app settings stylesheet failed"
    );
    ESP_BROOKESIA_CHECK_FALSE_RETURN(
        app_settings->activateStylesheet(app_settings_stylesheet), false, "Activate app settings stylesheet failed"
    );
    // app_settings->getPort().registerSetMediaDisplayBrightnessCallback(settings_port_set_media_display_brightness);
    // app_settings->getPort().registerGetMediaDisplayBrightnessCallback(settings_port_get_media_display_brightness);
    // app_settings->getPort().registerSetMediaSoundVolumeCallback(settings_port_set_media_sound_volume);
    // app_settings->getPort().registerGetMediaSoundVolumeCallback(settings_port_get_media_sound_volume);
    auto app_settings_id = speaker->installApp(app_settings);
    ESP_BROOKESIA_CHECK_FALSE_RETURN(speaker->checkAppID_Valid(app_settings_id), false, "Install app settings failed");

    speaker->getDisplay().getQuickSettings().on_event_signal.connect([=](QuickSettings::EventType event_type) {
        if (event_type != QuickSettings::EventType::SettingsButtonClicked) {
            return true;
        }

        if (speaker->getCoreManager().getRunningAppById(app_settings_id) != nullptr) {
            speaker->getManager().processQuickSettingsScrollTop();
            return true;
        }

        boost::thread([=]() {
            boost::this_thread::sleep_for(boost::chrono::microseconds(10));
            speaker->lockLv();
            ESP_Brookesia_CoreAppEventData_t event_data = {
                .id = app_settings_id,
                .type = ESP_BROOKESIA_CORE_APP_EVENT_TYPE_START,
                .data = nullptr
            };
            speaker->sendAppEvent(&event_data);
            speaker->unlockLv();
        }).detach();

        return true;
    });

    // lv_timer_t *timer = lv_timer_create([](lv_timer_t *t){
    //     auto speaker = static_cast<Speaker *>(t->user_data);
    //     if (speaker == nullptr) {
    //         ESP_BROOKESIA_LOGE("Speaker is null");
    //         return;
    //     }

    //     static int expression_index = FACE_MAX - 1;
    //     robot_face_type_t expressions[FACE_MAX] = {
    //         FACE_RECT,
    //         FACE_WRONGED,
    //         FACE_FURIOUS,
    //         FACE_GLAD,
    //         FACE_CUTE
    //     };
    //     speaker->getDisplay().getAI_Face()->setExpression(expressions[expression_index--]);
    //     if (expression_index < 0) {
    //         expression_index = FACE_MAX - 1;
    //     }
    // }, 5000, speaker);

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
