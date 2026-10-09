
cmake_minimum_required(VERSION 4.2.3)

#set(LV_BUILD_EXAMPLES 0)
#set(LV_BUILD_DEMOS 0)

# Disable LVGL examples and demos
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
#set(LV_BUILD_CONF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/)

# Option A: point directly to the file
set(LV_BUILD_CONF_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h"
    CACHE PATH "" FORCE)

# or Option B: point to the directory that contains lv_conf.h
# set(LV_BUILD_CONF_DIR
#     "${CMAKE_CURRENT_SOURCE_DIR}/main/uvm/app"
#     CACHE PATH "" FORCE)

# target_link_libraries(lvgl INTERFACE app_base)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv32im -mabi=ilp32" CACHE STRING "" FORCE)

add_subdirectory(../base base)

add_subdirectory(lvgl lvgl)
target_link_libraries(lvgl PUBLIC crt0)

add_executable(lvgl-app main.c)
target_link_libraries(lvgl-app PRIVATE lvgl)

add_custom_command(
    TARGET lvgl-app
    POST_BUILD
    COMMAND ${UTL_OBJCOPY} -O binary
            $<TARGET_FILE:lvgl-app>
            $<TARGET_FILE:lvgl-app>.bin
)


# add_custom_command(
#     TARGET lvgl-app
#     POST_BUILD
#     COMMAND ${UTL_OBJCOPY} -O binary
#             $<TARGET_FILE:lvgl-app>
#             $<TARGET_FILE:lvgl-app>.bin
#     COMMAND xxd -i -n lvgl-app_bin
#             $<TARGET_FILE:lvgl-app>.bin
#             | sed 's/unsigned char/const unsigned char/'
#             > ${CMAKE_CURRENT_SOURCE_DIR}/$<TARGET_FILE_NAME:lvgl-app>_bin.h
# )
