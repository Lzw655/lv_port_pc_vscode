cmake_minimum_required(VERSION 3.12.4)
project(lvgl C CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Set the correct FreeRTOS port for your system (e.g., Posix for WSL)
set(FREERTOS_PORT GCC_POSIX CACHE STRING "Port for FreeRTOS on Posix environment")
# Set the target Brookesia system type: Phone or Speaker
set(TARGET_BROOKESIA_SYSTEM Speaker CACHE STRING "Target Brookesia system type: Phone or Speaker")
set_property(CACHE TARGET_BROOKESIA_SYSTEM PROPERTY STRINGS Phone Speaker)

unset(USE_FREERTOS CACHE)
unset(USE_ESP_BROOKESIA CACHE)
unset(USE_ESP_BROOKESIA_APPS CACHE)

# add_compile_definitions(SDL_MAIN_HANDLED)
# add_compile_definitions(main=SDL_main)

option(USE_FREERTOS "Enable FreeRTOS" OFF) # Turn this on to enable FreeRTOS
option(USE_ESP_BROOKESIA "Enable ESP-Brookesia" ON) # Turn this on to enable ESP-Brookesia
option(USE_ESP_BROOKESIA_APPS "Enable ESP-Brookesia Apps" ON) # Turn this on to enable ESP-Brookesia

if(USE_FREERTOS)
    message(STATUS "FreeRTOS is enabled")

    # FreeRTOS integration when USE_FREERTOS is enabled
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/config)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

    # Add FreeRTOS as a subdirectory
    add_subdirectory(FreeRTOS)

    # FreeRTOS-specific include directories
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/include)
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix)
    include_directories(${PROJECT_SOURCE_DIR}/config)

    # Add FreeRTOS sources
    file(GLOB FREERTOS_SOURCES
        "${PROJECT_SOURCE_DIR}/FreeRTOS/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/MemMang/heap_4.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix/*.c"
    )
else()
    message(STATUS "FreeRTOS is disabled")
    set(FREERTOS_SOURCES "")  # No FreeRTOS sources if FreeRTOS is disabled
endif()

if(USE_ESP_BROOKESIA)
    message(STATUS "ESP-Brookesia is enabled")

    add_subdirectory(components/esp-lib-utils)

    # Common configurations
    set(CONFIG_ESP_BROOKESIA_CONF_ENABLE_GUI 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
    set(CONFIG_ESP_BROOKESIA_CONF_GUI_SQUARELINE_ENABLE_UI_COMP 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
    set(CONFIG_ESP_BROOKESIA_CONF_GUI_SQUARELINE_ENABLE_UI_HELPERS 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
    set(CONFIG_ESP_BROOKESIA_CONF_ENABLE_SYSTEMS 1 CACHE BOOL "Enable GUI in ESP-Brookesia")

    if (${TARGET_BROOKESIA_SYSTEM} STREQUAL "Phone")
        set(CONFIG_ESP_BROOKESIA_CONF_SYSTEMS_ENABLE_PHONE 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_PHONE_ENABLE_APP_EXAMPLES 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_PHONE_APP_EXAMPLES_ENABLE_SIMPLE_CONF 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_PHONE_APP_EXAMPLES_ENABLE_COMPLEX_CONF 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_PHONE_APP_EXAMPLES_ENABLE_SQUARELINE 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
    elseif (${TARGET_BROOKESIA_SYSTEM} STREQUAL "Speaker")
        set(CONFIG_ESP_BROOKESIA_CONF_ENABLE_AI_FRAMEWORK 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_AI_ENABLE_HMI 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_HMI_ENABLE_ROBOT_FACE 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_SYSTEMS_ENABLE_SPEAKER 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_SPEAKER_ENABLE_APP_EXAMPLES 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_SPEAKER_APP_EXAMPLES_ENABLE_SIMPLE_CONF 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
        set(CONFIG_ESP_BROOKESIA_CONF_SPEAKER_APP_EXAMPLES_ENABLE_COMPLEX_CONF 1 CACHE BOOL "Enable GUI in ESP-Brookesia")
    else()
        message(FATAL_ERROR "Invalid target Brookesia system type: ${TARGET_BROOKESIA_SYSTEM}")
    endif()
    set(ESP_BROOKESIA_DIR components/esp-brookesia)
    add_subdirectory(${ESP_BROOKESIA_DIR})

    if(USE_ESP_BROOKESIA_APPS)
        message(STATUS "ESP-Brookesia Apps is enabled")
        set(ESP_BROOKESIA_APPS_DIR components/esp-brookesia-apps/src/speaker/settings)
        add_subdirectory(${ESP_BROOKESIA_APPS_DIR})
        target_compile_options(speaker_settings
            PUBLIC
                -DESP_BROOKESIA_APP_SPEAKER_SETTINGS_SIMULATOR -DESP_BROOKESIA_APPS_SPEAKER_SETTINGS_KCONFIG_IGNORE
                -DESP_BROOKESIA_APPS_SPEAKER_SETTINGS_CONF_SKIP
        )
    else()
        message(STATUS "ESP-Brookesia Apps is disabled")
    endif()
else()
    message(STATUS "ESP-Brookesia is disabled")
endif()

# Main include files of the project
include_directories(${PROJECT_SOURCE_DIR}/main/inc)

# Define options for LVGL with default values (OFF)
option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
option(LV_USE_FREETYPE "Use freetype library" OFF)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output path for the executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Find and include SDL2 library
find_package(SDL2 REQUIRED)

# Find and include Boost library
find_package(Boost REQUIRED COMPONENTS thread chrono system)
include_directories(${Boost_INCLUDE_DIRS})

# 设置SDL2不使用SDL2main库
set(SDL_MAIN_HANDLED TRUE)
set(SDL2_SKIP_MAIN TRUE)

# Remove ARM-specific compile and linker options
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

if(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--undefined=main")
endif()

# Add compile definitions based on the selected options
add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)

# Add LVGL subdirectory
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL 1 CACHE BOOL "Disable ThorVG internal")
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1 CACHE BOOL "Disable examples")
set(LV_CONF_BUILD_DISABLE_DEMOS 1 CACHE BOOL "Disable demos")
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

set(MAIN_SRCS ${PROJECT_SOURCE_DIR}/main/src/main.cpp ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c)
if(USE_FREERTOS)
    list(APPEND MAIN_SRCS
        ${PROJECT_SOURCE_DIR}/main/src/freertos_main.cpp
        ${PROJECT_SOURCE_DIR}/main/src/FreeRTOS_Posix_Port.c
        ${FREERTOS_SOURCES}  # Add only if USE_FREERTOS is enabled
    )
endif()
if(USE_ESP_BROOKESIA)
    if(${TARGET_BROOKESIA_SYSTEM} STREQUAL "Phone")
        list(APPEND MAIN_SRCS
            ${PROJECT_SOURCE_DIR}/main/src/phone.cpp
        )
    elseif(${TARGET_BROOKESIA_SYSTEM} STREQUAL "Speaker")
        list(APPEND MAIN_SRCS
            ${PROJECT_SOURCE_DIR}/main/src/speaker.cpp
        )
    endif()
endif()

# Create the main executable, depending on the FreeRTOS option
add_executable(main ${MAIN_SRCS})

if(USE_FREERTOS)
    # Link FreeRTOS libraries
    target_link_libraries(main freertos_config FreeRTOS)
endif()
if(USE_ESP_BROOKESIA)
    # Link ESP-Brookesia libraries
    target_link_libraries(main esp-brookesia)
    target_include_directories(main PUBLIC ${ESP_BROOKESIA_DIR})

    if(USE_ESP_BROOKESIA_APPS)
        target_link_libraries(main speaker_settings)
        target_include_directories(main PUBLIC ${ESP_BROOKESIA_APPS_DIR})
    endif()
endif()

# Define LVGL configuration as a simple include
target_compile_definitions(main PRIVATE LV_CONF_INCLUDE_SIMPLE SDL_MAIN_HANDLED)
# target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} m pthread)
target_link_libraries(main lvgl ${SDL2_LIBRARIES} ${Boost_LIBRARIES} m pthread)

# Only link freertos_config if the FreeRTOS directory exists
if(USE_FREERTOS)
    target_link_libraries(main freertos_config)
endif()

# Custom target to run the executable
add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)

# Conditionally include and link SDL2_image if LV_USE_DRAW_SDL is enabled
if(LV_USE_DRAW_SDL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    find_package(SDL2_image REQUIRED)
    target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(main ${SDL2_IMAGE_LIBRARIES})
endif()

# Conditionally include and link libpng if LV_USE_LIBPNG is enabled
if(LV_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIRS})
    target_link_libraries(main ${PNG_LIBRARIES})
endif()

# Conditionally include and link libjpeg-turbo if LV_USE_LIBJPEG_TURBO is enabled
if(LV_USE_LIBJPEG_TURBO)
    find_package(JPEG REQUIRED)
    target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
    target_link_libraries(main ${JPEG_LIBRARIES})
endif()

# Conditionally include and link FFmpeg libraries if LV_USE_FFMPEG is enabled
if(LV_USE_FFMPEG)
    target_link_libraries(main avformat avcodec avutil swscale z)
endif()

# Conditionally include and link FreeType if LV_USE_FREETYPE is enabled
if(LV_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(main ${FREETYPE_LIBRARIES})
endif()

# Apply additional compile options if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode enabled")

    target_compile_options(lvgl PRIVATE
        -pedantic-errors
        -Wall
        -Wclobbered
        -Wdeprecated
        -Wdouble-promotion
        -Wempty-body
        -Wextra
        -Wformat-security
        -Wmaybe-uninitialized
        # -Wmissing-prototypes
        -Wpointer-arith
        -Wmultichar
        -Wno-pedantic # ignored for now, we convert functions to pointers for properties table.
        -Wreturn-type
        -Wshadow
        -Wshift-negative-value
        -Wsizeof-pointer-memaccess
        -Wtype-limits
        -Wundef
        -Wuninitialized
        -Wunreachable-code
        -Wfloat-conversion
        -Wstrict-aliasing
    )

if (ASAN)
    message(STATUS "AddressSanitizer enabled")

    # Add AddressSanitizer flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
else()
    message(STATUS "AddressSanitizer disabled")
endif()
endif()
