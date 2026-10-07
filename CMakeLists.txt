if(ESP_PLATFORM)
idf_component_register(
    SRCS
        "src/events.c"
        "src/utils.c"
        "uvm32/uvm32/uvm32.c"
    INCLUDE_DIRS
        "inc"
        "uvm32/uvm32"
        "uvm32/common"
        "uvm32/hosts/host-gdbstub"
    REQUIRES
        lwip
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    UVM32_MEMORY_SIZE=${CONFIG_UVMKIT_UVM32_MEMORY_SIZE}
    UVM32_ERROR_STRINGS
)

else()

cmake_minimum_required(VERSION 3.20)

project(uvmkit-project
        VERSION 1.0
        DESCRIPTION "UVMKIT Project"
        LANGUAGES C ASM)

set(UVM32_MEMORY_SIZE 1048576 CACHE STRING "UVM32 memory size in bytes")

# -- uvm32 --
add_library(uvm32 STATIC "./uvm32/uvm32/uvm32.c")
target_include_directories(uvm32 PUBLIC "./uvm32/uvm32" "./uvm32/common")
target_compile_definitions(uvm32 PUBLIC UVM32_MEMORY_SIZE=${UVM32_MEMORY_SIZE})
target_compile_definitions(uvm32 PUBLIC UVM32_ERROR_STRINGS)

# -- gdbstub --
add_library(gdbstub INTERFACE) # "./uvm32/hosts/host-gdbstub/gdbstub.h")
target_include_directories(gdbstub INTERFACE
                                   "./uvm32/hosts/host-gdbstub")
target_compile_definitions(gdbstub INTERFACE
                                   GDBSTUB_ARCH_UVM32
                                   GDBSTUB_IMPLEMENTATION)

# -- uvmkit --
add_library(uvmkit STATIC "./src/events.c" "./src/utils.c")
target_include_directories(uvmkit PUBLIC "./inc")
target_link_libraries(uvmkit uvm32 gdbstub)

endif()
