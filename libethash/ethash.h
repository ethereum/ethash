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

#define REVISION 15
#define DAGSIZE_BYTES_INIT 1073741824U
#define EPOCH_LENGTH 30000U
#define MIX_BYTES 4096
#define HASH_BYTES 64
#define DAG_PARENTS 8192
#define CACHE_ROUNDS 2

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ethash_params
{
    uint64_t full_size;				// Size of full data set (in bytes, multiple of mix size (128)).
    uint64_t cache_size;				// Size of compute cache (in bytes, multiple of node size (64)).
	uint32_t hash_read_size;
} ethash_params;

uint32_t ethash_get_datasize(const uint32_t block_number);
uint32_t ethash_get_cachesize(const uint32_t block_number);

// initialize the parameters
static inline void ethash_params_init(ethash_params* params, const uint32_t block_number)
{
	params->full_size = ethash_get_datasize(block_number);
    params->cache_size = ethash_get_cachesize(block_number);
	params->hash_read_size = 8*1024;
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

void ethash_mkcache(ethash_cache *cache, ethash_params const *params, const uint8_t seed[32]);
void ethash_compute_full_data(void *mem, ethash_params const *params, ethash_cache const *cache);
void ethash_full(uint8_t ret[32], void const *full_mem, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce);
void ethash_light(uint8_t ret[32], ethash_cache const *cache, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce);

#ifdef __cplusplus
}
#endif
