cmake_minimum_required(VERSION 4.2.3)

project(riscv-webserver VERSION 1.0
        DESCRIPTION "RISC-V webserver app"
        LANGUAGES C ASM)

add_subdirectory(../base base)

add_executable(echo echo.c)
target_link_libraries(echo PRIVATE crt0 uvmkit_socket)

add_custom_command(
    TARGET echo
    POST_BUILD
    COMMAND ${UTL_OBJCOPY} -O binary
            $<TARGET_FILE:echo>
            $<TARGET_FILE:echo>.bin
)
