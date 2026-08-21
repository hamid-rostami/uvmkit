cmake_minimum_required(VERSION 4.2.3)

project(linux-uvmkit-host
        VERSION 1.0
        DESCRIPTION "Linux UVMKIT host"
        LANGUAGES C ASM)

add_subdirectory("../../" uvmkit)
add_executable(host host.c)
target_link_libraries(host uvmkit)
