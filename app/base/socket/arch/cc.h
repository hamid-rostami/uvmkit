#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   u8_t;
typedef int8_t    s8_t;

typedef uint16_t  u16_t;
typedef int16_t   s16_t;

typedef uint32_t  u32_t;
typedef int32_t   s32_t;

typedef uintptr_t mem_ptr_t;

#define BYTE_ORDER LITTLE_ENDIAN

#define LWIP_PLATFORM_DIAG(x)
#define LWIP_PLATFORM_ASSERT(x)
