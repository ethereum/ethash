/*
  This file is part of cpp-ethereum.

  cpp-ethereum is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  cpp-ethereum is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file ethash.h
 * @author Tim Hughes <tim@ethdev.org>
 * @date 2015
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ethash_params
{
	unsigned full_size;					// Size of full data set (in bytes, multiple of page size (4096)).
	unsigned cache_size;				// Size of compute cache (in bytes, multiple of node size (64)).
	unsigned hash_read_size;			// Size of data set to read for each hash (in bytes, multiple of page size (4096)).
	unsigned k;							// Number of parents of a full node.
	uint8_t seed[32];					// Seed for data set.
} ethash_params;

// init to defaults
static inline void ethash_params_init(ethash_params* params)
{
	params->full_size = 262147 * 4096;	// 1GB-ish;
	params->cache_size = 8209 * 4096;	// 32MB-ish
	params->hash_read_size = 32 * 4096;	// 128k
	params->k = 64;
	memset(params->seed, 0, sizeof(params->seed));
}

typedef struct ethash_cache
{
	void* mem;
	uint32_t rng_table[32];
} ethash_cache;

static inline void ethash_cache_init(ethash_cache* cache, void* mem)
{
	memset(cache, 0, sizeof(*cache));
	cache->mem = mem;
}

void ethash_mkcache(ethash_cache *cache, ethash_params const *params);
void ethash_compute_full_data(void* mem, ethash_params const* params);
void ethash_full(uint8_t ret[32], void const* full_mem, ethash_params const* params, uint64_t nonce);
void ethash_light(uint8_t ret[32], ethash_cache const* cache, ethash_params const* params, uint64_t nonce);

#ifdef __cplusplus
}
#endif
