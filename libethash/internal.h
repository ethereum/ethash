#pragma once
#include "compiler.h"
#include "endian.h"
#include "ethash.h"

// todo: flag not needed?
#define ENABLE_SSE 1

#if defined(_M_X64) && ENABLE_SSE
#include <smmintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// compile time settings
#define NODE_WORDS (64/4)
#define PAGE_WORDS (4096/4)
#define PAGE_NODES (PAGE_WORDS / NODE_WORDS)
#define CACHE_ROUNDS 2
#include <stdint.h>

typedef union node {
    uint8_t bytes[NODE_WORDS*4];
    uint32_t words[NODE_WORDS];
    uint64_t uint64s[NODE_WORDS/2];

#if defined(_M_X64) && ENABLE_SSE
	__m128i xmm[NODE_WORDS/4];
#endif

} node;

void static ethash_compute_full_node(
        node *const ret,
        const unsigned node_index,
        ethash_params const *params,
        node const *cache,
        uint32_t const *rng_table
);

#ifdef __cplusplus
}
#endif