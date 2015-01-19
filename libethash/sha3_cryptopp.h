#pragma once

#include "compiler.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void sha3_256(uint8_t* const ret, const uint8_t * data, uint32_t size);
void sha3_512(uint8_t* const ret, const uint8_t * data, uint32_t size);

#ifdef __cplusplus
}
#endif