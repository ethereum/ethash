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
/** @file dash.cpp
* @author Tim Hughes <tim@ethdev.org>
* @date 2015
*/

#include <assert.h>
#include "ethash.h"
#include "blum_blum_shub.h"
#include "fnv.h"
#include "endian.h"
#include "internal.h"

#ifdef WITH_CRYPTOPP
#include "SHA3_cryptopp.h"
#else
#include "sha3.h"
#endif // WITH_CRYPTOPP


// Follows Sergio's "STRICT MEMORY HARD HASHING FUNCTIONS" (2014)
// https://bitslog.files.wordpress.com/2013/12/memohash-v0-3.pdf
// SeqMemoHash(s, R, N)
void static ethash_compute_cache_nodes(node *const nodes, ethash_params const *params, const uint8_t seed[32]) {
    assert((params->cache_size % sizeof(node)) == 0);
    unsigned const num_nodes = params->cache_size / sizeof(node);

    SHA3_512(nodes[0].bytes, seed, 32);

    for (unsigned i = 1; i != num_nodes; ++i) {
        SHA3_512(nodes[i].bytes, nodes[i - 1].bytes, 64);
    }

    for (unsigned j = 0; j != CACHE_ROUNDS; j++) {
        for (unsigned i = 0; i != num_nodes; ++i) {
            uint32_t const idx = (uint32_t)(fix_endian64(nodes[i].uint64s[0]) % num_nodes);
            node data[2];
			data[0] = nodes[(num_nodes-1+i) % num_nodes];
			data[1] = nodes[idx];
            SHA3_512(nodes[i].bytes, data[0].bytes, sizeof(data));
        };
	}

	// now perform endian conversion
#if BYTE_ORDER != LITTLE_ENDIAN
	for (unsigned w = 0; w != (num_nodes*NODE_WORDS); ++w)
	{
		nodes->words[w] = fix_endian32(nodes->words[w]);
	}
#endif
}

void ethash_mkcache(ethash_cache *cache, ethash_params const *params, const uint8_t seed[32]) {
    node *nodes = (node *) cache->mem;
    ethash_compute_cache_nodes(nodes, params, seed);

	// todo, do we need 64-bits?
	uint32_t const low_word = nodes[0].words[0];
	uint32_t const high_word = nodes[0].words[1];
    uint32_t rng_seed = make_seed1((uint64_t)high_word << 32 | low_word);
    init_power_table_mod_prime1(cache->rng_table, rng_seed);
}

void static ethash_compute_full_node(
        node *const ret,
        const unsigned node_index,
        ethash_params const *params,
        node const *cache,
        uint32_t const *rng_table
) {
    size_t num_parent_nodes = params->cache_size / sizeof(node);

	node const* init = &cache[node_index % num_parent_nodes];
#if defined(_M_X64) && ENABLE_SSE
	__m128i const fnv_prime = _mm_set1_epi32(FNV_PRIME);
	__m128i xmm0 = init->xmm[0];
	__m128i xmm1 = init->xmm[1];
	__m128i xmm2 = init->xmm[2];
	__m128i xmm3 = init->xmm[3];
#else
    memcpy(ret, init, sizeof(node));
#endif

    uint32_t rand = make_seed2(quick_bbs(rng_table, node_index));
    for (unsigned i = 0; i != DAG_PARENTS; ++i) {

        const size_t parent_index = (rand ^ ret->words[i % NODE_WORDS]) % num_parent_nodes;

        rand = cube_mod_safe_prime2(rand);

        node const* parent = &cache[parent_index];

#if defined(_M_X64) && ENABLE_SSE
		{
			xmm0 = _mm_mullo_epi32(xmm0, fnv_prime);
			xmm1 = _mm_mullo_epi32(xmm1, fnv_prime);
			xmm2 = _mm_mullo_epi32(xmm2, fnv_prime);
			xmm3 = _mm_mullo_epi32(xmm3, fnv_prime);
			xmm0 = _mm_xor_si128(xmm0, parent->xmm[0]);
			xmm1 = _mm_xor_si128(xmm1, parent->xmm[1]);
			xmm2 = _mm_xor_si128(xmm2, parent->xmm[2]);
			xmm3 = _mm_xor_si128(xmm3, parent->xmm[3]);
		}
		#else
        {
            for (unsigned w = 0; w != NODE_WORDS; ++w) {
                ret->words[w] = fnv_hash(ret->words[w], parent->words[w]);
            }
        }
#endif
    }

#if defined(_M_X64) && ENABLE_SSE
	{
		ret->xmm[0] = xmm0;
		ret->xmm[1] = xmm1;
		ret->xmm[2] = xmm2;
		ret->xmm[3] = xmm3;
	}
	#endif
}

void ethash_compute_full_data(void *mem, ethash_params const *params, const uint8_t seed[32]) {
    assert((params->full_size % (sizeof(uint64_t) * PAGE_WORDS)) == 0);
    assert((params->full_size % sizeof(node)) == 0);
    node *nodes = (node *) mem;

    // compute cache nodes first
    ethash_cache cache;
    ethash_cache_init(&cache, mem);
    ethash_mkcache(&cache, params, seed);

    // now compute full nodes
    for (unsigned n = params->cache_size / sizeof(node); n != params->full_size / sizeof(node); ++n) {
        ethash_compute_full_node(&nodes[n], n, params, nodes, cache.rng_table);
    }
}

static void ethash_hash(
        uint8_t ret[32],
        node const * nodes,
        uint32_t const *rng_table,
        ethash_params const *params,
        const uint8_t previous_hash[32],
        const uint64_t nonce) {
    // for simplicity the cache and dag must be whole number of pages
    assert((params->cache_size % PAGE_WORDS) == 0);
    assert((params->full_size % PAGE_WORDS) == 0);

    // pack previous_hash and nonce together
    struct {
        uint8_t previous_hash[32];
        uint64_t nonce;
    } init;
    memcpy(init.previous_hash, previous_hash, 32);
    init.nonce = fix_endian64(nonce);

    // compute sha3-256 hash and replicate across mix
    node mix[PAGE_NODES];
    SHA3_256(mix->bytes, (uint8_t const *) &init, sizeof(init));
#if BYTE_ORDER != LITTLE_ENDIAN
	for (unsigned w = 0; w != 8; ++w) {
		mix->words[w] = fix_endian32(mix->words[w]);
	}
#endif
    for (unsigned w = 8; w != PAGE_WORDS; ++w) {
        mix->words[w] = mix->words[w % 8];
    }

	// todo, do we need 64-bits?
    uint32_t const low_word = mix->words[0];
	uint32_t const high_word = mix->words[1];
    uint32_t rand = make_seed1((uint64_t)high_word << 32 | low_word);

    unsigned const
            page_size = sizeof(uint32_t) * PAGE_WORDS,
            accesses = params->hash_read_size / page_size,
            num_full_pages = params->full_size / page_size,
            num_cache_pages = params->cache_size / page_size,
            num_pages = rng_table ? num_cache_pages : num_full_pages;

    for (unsigned i = 0; i != accesses; ++i) {
        uint32_t const index =  (rand ^ mix->words[i % PAGE_WORDS]) % num_full_pages;
        rand = cube_mod_safe_prime1(rand);

        node tmp_page[PAGE_NODES];
		node const*page;
        if (index >= num_pages) {
            page = tmp_page;

            for (unsigned n = 0; n != PAGE_NODES; ++n) {
                ethash_compute_full_node(&tmp_page[n], index * PAGE_NODES + n, params, nodes, rng_table);
            }
        }
        else {
            page = nodes + PAGE_NODES * index;
        }

		#if defined(_M_X64) && ENABLE_SSE
		{
			__m128i fnv_prime = _mm_set1_epi32(FNV_PRIME);
			for (unsigned n = 0; n != PAGE_NODES; ++n) {
				__m128i xmm0 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[0]);
				__m128i xmm1 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[1]);
				__m128i xmm2 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[2]);
				__m128i xmm3 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[3]);
				mix[n].xmm[0] = _mm_xor_si128(xmm0, page[n].xmm[0]);
				mix[n].xmm[1] = _mm_xor_si128(xmm1, page[n].xmm[1]);
				mix[n].xmm[2] = _mm_xor_si128(xmm2, page[n].xmm[2]);
				mix[n].xmm[3] = _mm_xor_si128(xmm3, page[n].xmm[3]);
			}
		}
		#else
		{
			for (unsigned w = 0; w != PAGE_WORDS; ++w) {
				mix->words[w] = fnv_hash(mix->words[w], page->words[w]);
			}
		}
		#endif
    }

#if BYTE_ORDER != LITTLE_ENDIAN
	for (unsigned w = 0; w != PAGE_WORDS; ++w) {
		mix->words[w] = fix_endian32(mix->words[w]);
	}
#endif

    uint8_t tmp[32];
    SHA3_256(tmp, mix->bytes, sizeof(mix));
    SHA3_256(ret, tmp, sizeof(tmp));
}

void ethash_full(uint8_t ret[32], void const *full_mem, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce) {
    ethash_hash(ret, (node const *) full_mem, NULL, params, previous_hash, nonce);
}

void ethash_light(uint8_t ret[32], ethash_cache const *cache, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce) {
    ethash_hash(ret, (node const *) cache->mem, cache->rng_table, params, previous_hash, nonce);
}