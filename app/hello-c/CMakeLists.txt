cmake_minimum_required(VERSION 4.2.3)

project(hello-riscv VERSION 1.0
        DESCRIPTION "RISC-V app"
        LANGUAGES C ASM)

add_subdirectory(../base base) # FIXME


add_executable(helloworld helloworld.c)
target_link_libraries(helloworld crt0)
target_compile_options(helloworld PRIVATE -g3 -O2 -Wall -Werror -Wno-unused-function)

add_custom_command(
    TARGET helloworld
    POST_BUILD
    COMMAND ${UTL_OBJCOPY} -O binary
            $<TARGET_FILE:helloworld>
            $<TARGET_FILE:helloworld>.bin
)
