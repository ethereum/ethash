#pragma once
#include "compiler.h"
#include "endian.h"
#include "ethash.h"
#include <stdio.h>

#define ENABLE_SSE 0

#if defined(_M_X64) && ENABLE_SSE
#include <smmintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// compile time settings
#define NODE_WORDS (64/4)
#define MIX_WORDS (MIX_BYTES/4)
#define MIX_NODES (MIX_WORDS / NODE_WORDS)
#include <stdint.h>

typedef union node {
	uint8_t bytes[NODE_WORDS * 4];
	uint32_t words[NODE_WORDS];
	uint64_t double_words[NODE_WORDS / 2];

#if defined(_M_X64) && ENABLE_SSE
	__m128i xmm[NODE_WORDS/4];
#endif

} node;

struct ethash_light {
	ethash_cache* cache;
};

struct ethash_full {
	FILE* file;
	size_t file_size;
	ethash_cache* cache;
	node* data;
	ethash_callback_t callback;
};

void ethash_calculate_dag_item(
	node* const ret,
	const unsigned node_index,
	ethash_params const* params,
	ethash_cache const* cache
);

void ethash_quick_hash(
	ethash_h256_t* return_hash,
	ethash_h256_t const* header_hash,
	const uint64_t nonce,
	ethash_h256_t const* mix_hash
);

#ifdef __cplusplus
}
#endif
