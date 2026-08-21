enable_language(ASM)

add_library(app_base INTERFACE)
target_include_directories(
    app_base INTERFACE
    "."
    "../../inc/uvmkit/common" # app and host common
    "../../uvm32/common" # uvm32
)

target_compile_options(
    app_base
    INTERFACE
    -march=rv32im
    -mabi=ilp32
    -ffreestanding
    -fdata-sections
    -ffunction-sections
    -fno-stack-protector
    -g3
    -Os
    --specs=nano.specs
)

target_link_options(
    app_base
    INTERFACE
    -march=rv32im
    -mabi=ilp32
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    -nostartfiles
    --specs=nano.specs
    -Wl,--gc-sections
)

add_library(crt0 OBJECT crt0.S syscalls.c)
target_link_libraries(crt0 app_base) # mpack)


add_library(uvmkit_socket STATIC socket/socket.c)
target_include_directories(uvmkit_socket PUBLIC
    "socket/inc"
)

target_link_libraries(uvmkit_socket PUBLIC crt0)
