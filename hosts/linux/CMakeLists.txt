cmake_minimum_required(VERSION 3.28)

project(linux-uvmkit-host
        VERSION 1.0
        DESCRIPTION "Linux UVMKIT host"
        LANGUAGES C ASM)

find_package(raylib CONFIG REQUIRED)
find_package(libwebsockets CONFIG REQUIRED)

add_subdirectory("../../" uvmkit)
add_executable(host host.c ws.c)
target_link_libraries(host uvmkit websockets)

