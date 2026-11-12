cmake_minimum_required(VERSION 3.20)

project(riscv-webserver VERSION 1.0
        DESCRIPTION "RISC-V webserver app"
        LANGUAGES C ASM)

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

add_executable(echo echo.c)
target_link_libraries(echo PRIVATE crt0 uvmkit_socket)

add_custom_command(
    TARGET echo
    POST_BUILD
    COMMAND ${UTL_OBJCOPY} -O binary
            $<TARGET_FILE:echo>
            $<TARGET_FILE:echo>.bin
)
