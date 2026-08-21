#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

static inline void
uvmkit_encode_u32(uint32_t *out, uint32_t in) {
  uint8_t* p = (uint8_t*) out;
  *p++ = (uint8_t)in;
  *p++ = (uint8_t)(in >> 8);
  *p++ = (uint8_t)(in >> 16);
  *p++ = (uint8_t)(in >> 24);
}

static inline uint32_t
uvmkit_decode_u32(uint32_t *in) {
  uint32_t out = 0;
  uint8_t* p = (uint8_t*) in;
  out = (uint32_t)*p;
  p++;
  out += (uint32_t)*p << 8;
  p++;
  out += (uint32_t)*p << 16;
  p++;
  out += (uint32_t)*p << 24;
  return out;
}
