#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"
#include <stdint.h>
#include <stdlib.h>

#define decshake(bits) \
  int shake##bits(uint8_t*, size_t, const uint8_t*, size_t);

#define decsha3(bits) \
  int sha3_##bits(uint8_t*, size_t, const uint8_t*, size_t);

decshake(128)
decshake(256)
decsha3(224)
decsha3(256)
decsha3(384)
decsha3(512)

static inline void SHA3_256(void *const ret, void const *data, const uint32_t size) {
    sha3_256(ret, 32, data, size);
}

static inline void SHA3_512(void *const ret, void const *data, const uint32_t size) {
    sha3_512(ret, 64, data, size);
}

#ifdef __cplusplus
}
#endif