# Usage:
# cmake -DCMAKE_TOOLCHAIN_FILE=riscv-toolchain.cmake ..

set(CMAKE_SYSTEM_NAME Generic)
set(TOOLCHAIN_PREFIX riscv64-unknown-elf-)
set(GCC_SPECS nano.specs)
#set(GCC_SPECS picolibc.specs)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
#set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp -march=rv32im -mabi=ilpb32")

set(UTL_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(UTL_SIZE ${TOOLCHAIN_PREFIX}size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)