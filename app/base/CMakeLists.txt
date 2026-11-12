enable_language(ASM)

get_filename_component(UVMKIT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_library(app_base INTERFACE)
target_include_directories(
    app_base INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${UVMKIT_ROOT}/inc/uvmkit/common" # app and host common
    "${UVMKIT_ROOT}/uvm32/common" # uvm32
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
    --specs=${GCC_SPECS}
)

target_link_options(
    app_base
    INTERFACE
    -march=rv32im
    -mabi=ilp32
    -T${CMAKE_CURRENT_LIST_DIR}/linker.ld
    -nostartfiles
    --specs=${GCC_SPECS}
    -Wl,--gc-sections
)

add_library(crt0 OBJECT ${CMAKE_CURRENT_LIST_DIR}/crt0.S ${CMAKE_CURRENT_LIST_DIR}/syscalls.c)
target_link_libraries(crt0 app_base) # mpack)


add_library(uvmkit_socket STATIC ${CMAKE_CURRENT_LIST_DIR}/socket/socket.c)
target_include_directories(uvmkit_socket PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/socket/inc"
)

target_link_libraries(uvmkit_socket PUBLIC crt0)

# Helper function to link crt0 and generate the raw .bin artifact
function(uvmkit_add_app TARGET_NAME)
    target_link_libraries(${TARGET_NAME} PRIVATE crt0)
    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${UTL_OBJCOPY} -O binary
                $<TARGET_FILE:${TARGET_NAME}>
                $<TARGET_FILE:${TARGET_NAME}>.bin
        COMMENT "Generating binary $<TARGET_FILE_NAME:${TARGET_NAME}>.bin"
    )
endfunction()
