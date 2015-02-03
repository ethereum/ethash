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
#include <stdio.h>
#include "ethash.h"
#include "blum_blum_shub.h"
#include "fnv.h"
#include "endian.h"
#include "internal.h"
#include "nth_prime.h"

#ifdef WITH_CRYPTOPP
#include "SHA3_cryptopp.h"

#else
#include "sha3.h"
#endif // WITH_CRYPTOPP

uint32_t ethash_get_datasize(const uint32_t block_number) {
    uint32_t datasize = (DAGSIZE_BYTES_INIT + DAGSIZE_BYTES_GROWTH * block_number) / (EPOCH_LENGTH * MIX_BYTES);
    uint32_t i = 23000;
    while(nth_prime(i++) < datasize);
    return nth_prime(i-1) * MIX_BYTES;
};

uint32_t ethash_get_cachesize(const uint32_t block_number) {
    uint32_t
            cachesize = ethash_get_datasize(block_number) / (32 * HASH_BYTES),
            i = 0;
    while(nth_prime(i++) < cachesize);
    return nth_prime(i) * HASH_BYTES;
};

// Follows Sergio's "STRICT MEMORY HARD HASHING FUNCTIONS" (2014)
// https://bitslog.files.wordpress.com/2013/12/memohash-v0-3.pdf
// SeqMemoHash(s, R, N)
void static ethash_compute_cache_nodes(node *const nodes, ethash_params const *params, const uint8_t seed[32]) {
    assert((params->cache_size % sizeof(node)) == 0);
    assert((params->cache_size % sizeof(node)) == 0);
    unsigned const num_nodes = params->cache_size / sizeof(node);

    SHA3_512(nodes[0].bytes, seed, 32);

    for (unsigned i = 1; i != num_nodes; ++i) {
        SHA3_512(nodes[i].bytes, nodes[i - 1].bytes, 64);
    }

    for (unsigned j = 0; j != CACHE_ROUNDS; j++) {
        for (unsigned i = 0; i != num_nodes; i++) {
            uint32_t const idx = (uint32_t)(fix_endian64(nodes[i].uint64s[0]) % num_nodes);
            node data[2];
			data[0] = nodes[(num_nodes-1+i) % num_nodes];
			data[1] = nodes[idx];
            SHA3_512(nodes[i].bytes, data[0].bytes, sizeof(data));
        }
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
    init_power_table_mod_prime1(cache->rng_table, make_seed1(nodes[0].words[0]));
}

static void ethash_compute_full_node(
        node *const ret,
        const unsigned node_index,
        ethash_params const *params,
        ethash_cache const *cache
) {
    size_t num_parent_nodes = params->cache_size / sizeof(node);
	node const* cache_nodes = (node const*)cache->mem;

	node const* init = &cache_nodes[node_index % num_parent_nodes];
#if defined(_M_X64) && ENABLE_SSE
	__m128i const fnv_prime = _mm_set1_epi32(FNV_PRIME);
	__m128i xmm0 = init->xmm[0];
	__m128i xmm1 = init->xmm[1];
	__m128i xmm2 = init->xmm[2];
	__m128i xmm3 = init->xmm[3];
#else
    memcpy(ret, init, sizeof(node));
#endif

    uint32_t rand2 = make_seed2(quick_bbs(cache->rng_table, node_index));
    for (unsigned i = 0; i != DAG_PARENTS; ++i) {

        const size_t parent_index = (rand2 ^ ret->words[i % NODE_WORDS]) % num_parent_nodes;
        rand2 = cube_mod_safe_prime2(rand2);

        node const* parent = &cache_nodes[parent_index];

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

void ethash_compute_full_data(
        void *mem,
        ethash_params const *params,
        ethash_cache const* cache) {
    assert((params->full_size % (sizeof(uint64_t) * PAGE_WORDS)) == 0);
    assert((params->full_size % sizeof(node)) == 0);

    // now compute full nodes
    for (unsigned n = 0; n != params->full_size ; n += sizeof(node)) {
        ethash_compute_full_node(&(mem[n]), n, params, cache);
    }
}

static void ethash_hash(
        uint8_t ret[32],
        node const* full_nodes,
		ethash_cache const* cache,
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

    uint32_t rand2 = make_seed2(mix->words[0]);

    unsigned const
            page_size = sizeof(uint32_t) * PAGE_WORDS,
            accesses = params->hash_read_size / page_size,
            num_full_pages = params->full_size / page_size;

    for (unsigned i = 0; i != accesses; ++i) {
        uint32_t const index = (rand2 ^ mix->words[i % PAGE_WORDS]) % num_full_pages;
        rand2 = cube_mod_safe_prime2(rand2);

        // Todo: better to do one node of mix at a time and FNV directly into the xmm registers
        for (unsigned n = 0; n != PAGE_NODES; ++n) {
            if (!full_nodes) {
                node tmp_node;
                ethash_compute_full_node(&tmp_node, index * PAGE_NODES + n, params, cache);

#if defined(_M_X64) && ENABLE_SSE
			    	{
					__m128i fnv_prime = _mm_set1_epi32(FNV_PRIME);
					__m128i xmm0 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[0]);
					__m128i xmm1 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[1]);
					__m128i xmm2 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[2]);
					__m128i xmm3 = _mm_mullo_epi32(fnv_prime, mix[n].xmm[3]);
					mix[n].xmm[0] = _mm_xor_si128(xmm0, tmp_node.xmm[0]);
					mix[n].xmm[1] = _mm_xor_si128(xmm1, tmp_node.xmm[1]);
					mix[n].xmm[2] = _mm_xor_si128(xmm2, tmp_node.xmm[2]);
					mix[n].xmm[3] = _mm_xor_si128(xmm3, tmp_node.xmm[3]);
				}
			    	#else

                for (unsigned w = 0; w != NODE_WORDS; ++w) {
                    mix[n].words[w] = fnv_hash(mix[n].words[w], tmp_node.words[w]);
                }
#endif
            }
            else {
                node const *page = &full_nodes[PAGE_NODES * index + n];
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
                for (unsigned n = 0; n != PAGE_WORDS; ++n) {
                    mix->words[n] = fnv_hash(mix->words[n], page->words[n]);
                }
        #endif
            }
        }
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
    ethash_hash(ret, NULL, cache, params, previous_hash, nonce);
}