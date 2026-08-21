cmake_minimum_required(VERSION 4.2.3)

project(uvmkit-project
        VERSION 1.0
        DESCRIPTION "UVMKIT Project"
        LANGUAGES C ASM)

# -- uvm32 --
add_library(uvm32 STATIC "./uvm32/uvm32/uvm32.c")
target_include_directories(uvm32 PUBLIC "./uvm32/uvm32" "./uvm32/common")
target_compile_definitions(uvm32 PUBLIC UVM32_MEMORY_SIZE=1048576) # 1MB
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


# if(ESP_PLATFORM)
# else()
# endif()
