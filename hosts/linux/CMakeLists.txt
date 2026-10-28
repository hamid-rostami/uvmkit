cmake_minimum_required(VERSION 3.28)

project(linux-uvmkit-host
        VERSION 1.0
        DESCRIPTION "Linux UVMKIT host"
        LANGUAGES C ASM)

#find_package(raylib CONFIG REQUIRED)
find_package(libwebsockets CONFIG REQUIRED)

add_subdirectory("../../" uvmkit)
add_executable(host host.c ws.c)
target_link_libraries(host uvmkit websockets)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/index.html"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
    "${CMAKE_CURRENT_BINARY_DIR}/index.html"
)

# Create a target for the file copy
add_custom_target(copy_index ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/index.html"
)

# Ensure the host target waits for the copy to happen
add_dependencies(host copy_index)
