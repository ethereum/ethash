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

#define REVISION 10

#ifdef __cplusplus
extern "C" {
#endif



#define DAG_PARENTS 64

typedef struct ethash_params
{
    unsigned full_size;					// Size of full data set (in bytes, multiple of page size (4096)).
    unsigned cache_size;				// Size of compute cache (in bytes, multiple of node size (64)).
    unsigned hash_read_size;			// Size of data set to read for each hash (in bytes, multiple of page size (4096)).
} ethash_params;

// init to defaults
static inline void ethash_params_init(ethash_params* params)
{
	//params->full_size = 121349 * 8192; // 1GB-ish;
	params->full_size = 8663 * 8192; // 70MB-ish (for testing!);
	params->cache_size = 8209 * 4096;	// 32MB-ish
	params->hash_read_size = 32 * 4096;	// 128k
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

static inline int check_hash_less_than_difficulty(const uint8_t hash[32], const uint8_t difficulty[32]) {
    const uint64_t
            * hash_ = (uint64_t const *) hash,
            * difficulty_ = (uint64_t const *) difficulty;
     for (int i = 3; i >= 0 ; --i) {
        if (hash_[i] < difficulty_[i])
            return 1;
        if (hash_[i] > difficulty_[i])
            return 0;
    }
    return 0;
}

void ethash_mkcache(ethash_cache *cache, ethash_params const *params, const uint8_t seed[32]);
void ethash_compute_full_data(void *mem, ethash_params const *params, const uint8_t seed[32]);
void ethash_full(uint8_t ret[32], void const *full_mem, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce);
void ethash_light(uint8_t ret[32], ethash_cache const *cache, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce);

#ifdef __cplusplus
}
#endif
