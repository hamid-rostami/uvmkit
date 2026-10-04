cmake_minimum_required(VERSION 4.2.3)

project(riscv-webserver VERSION 1.0
        DESCRIPTION "RISC-V webserver app"
        LANGUAGES C ASM)

add_subdirectory(../base base)

#add_library(picohttpparser "picohttpparser/picohttpparser.c")
#target_include_directories(picohttpparser PUBLIC "picohttpparser")

add_executable(server server.c "picohttpparser/picohttpparser.c")
target_include_directories(server PUBLIC "picohttpparser")
target_link_libraries(server PRIVATE crt0 uvmkit_socket) # picohttpparser)

add_custom_command(
    TARGET server
    POST_BUILD
    COMMAND ${UTL_OBJCOPY} -O binary
            $<TARGET_FILE:server>
            $<TARGET_FILE:server>.bin
)
