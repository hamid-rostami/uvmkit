cmake_minimum_required(VERSION 3.20)

project(riscv-lvgl VERSION 1.0
        DESCRIPTION "RISC-V LVGL app"
        LANGUAGES C ASM)


# Disable LVGL examples and demos
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)

# Option A: point directly to the file
set(LV_BUILD_CONF_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h"
    CACHE PATH "" FORCE)

add_subdirectory(../base base)

add_subdirectory(lvgl lvgl)
target_link_libraries(lvgl PUBLIC crt0)

add_executable(lvgl-app main.c)
target_link_libraries(lvgl-app PUBLIC lvgl crt0)

add_custom_command(
    TARGET lvgl-app
    POST_BUILD
    COMMAND ${UTL_OBJCOPY} -O binary
            $<TARGET_FILE:lvgl-app>
            $<TARGET_FILE:lvgl-app>.bin
)
