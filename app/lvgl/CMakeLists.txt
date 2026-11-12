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

# Locate UVMKit base
if(NOT DEFINED UVMKIT_PATH)
    if(DEFINED ENV{UVMKIT_PATH})
        set(UVMKIT_PATH "$ENV{UVMKIT_PATH}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../base/CMakeLists.txt")
        get_filename_component(UVMKIT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
    else()
        message(FATAL_ERROR
            "Could not locate UVMKit!\n"
            "Please enter the nix-shell (nix-shell <path-to-uvmkit>/nix/shell.nix) "
            "or set UVMKIT_PATH."
        )
    endif()
endif()

add_subdirectory("${UVMKIT_PATH}/app/base" uvmkit_base)

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
