#pragma once

#include <stdint.h>
#include <stddef.h>

struct ethash_h256;
typedef struct ethash_h256 ethash_h256_t;

#ifdef __cplusplus
extern "C" {
#endif

void keccak256(ethash_h256_t* out, uint8_t const* data, size_t size);

void keccak512(uint8_t* out, uint8_t const* data, size_t size);

void keccak256_96(ethash_h256_t* out, uint8_t const* data);

void keccak512_40(uint8_t* out, uint8_t const* data);

#ifdef __cplusplus
}
#endif
