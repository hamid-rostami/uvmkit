cmake_minimum_required(VERSION 3.20)

project(hello-riscv VERSION 1.0
        DESCRIPTION "RISC-V app"
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

add_executable(helloworld helloworld.c)
target_compile_options(helloworld PRIVATE -g3 -O2 -Wall -Werror -Wno-unused-function)
uvmkit_add_app(helloworld)
