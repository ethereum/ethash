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

#include <stdio.h>
#include <assert.h>
#include "ethash.h"
#include "blum_blum_shub.h"
#include "fnv.h"
#include "endian.h"

#ifdef WITH_CRYPTOPP
#include "SHA3_cryptopp.h"
#else
#include "sha3.h"
#endif // WITH_CRYPTOPP

// compile time settings
#define NODE_WORDS (64/4)
#define PAGE_WORDS (4096/4)
#define PAGE_NODES (PAGE_WORDS / NODE_WORDS)
#define CACHE_ROUNDS 2

typedef union node {
    uint8_t bytes[NODE_WORDS*4];
    uint32_t words[NODE_WORDS];
} node;


// Follows Sergio's "STRICT MEMORY HARD HASHING FUNCTIONS" (2014)
// https://bitslog.files.wordpress.com/2013/12/memohash-v0-3.pdf
// SeqMemoHash(s, R, N)
void static ethash_compute_cache_nodes(node *const nodes, ethash_params const *params, const uint8_t seed[32]) {
    assert((params->cache_size % sizeof(node)) == 0);
    const size_t num_nodes = params->cache_size / sizeof(node);


    SHA3_512(nodes[0].bytes, seed, 32);

    for (unsigned i = 1; i != num_nodes; ++i) {
        SHA3_512(nodes[i].bytes, nodes[i - 1].bytes, 64);
    }

    for (unsigned j = 0; j != CACHE_ROUNDS; j++) {
        for (unsigned i = 0; i != num_nodes; ++i) {
			// is there a better way to compute the parent index?
			uint32_t const low_word = fix_endian32(nodes[i].words[0]);
			uint32_t const high_word = fix_endian32(nodes[i].words[1]);
            unsigned const idx = (unsigned)(((uint64_t)high_word << 32 | low_word) % num_nodes);

			// todo, better to use update calls than copying this data into one buffer.
            node data[2];
			data[0] = nodes[(i-1+num_nodes) % num_nodes];
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

	// todo, do we need 64-bits?
	uint32_t const low_word = nodes[0].words[0];
	uint32_t const high_word = nodes[0].words[1];
    uint32_t rng_seed = make_seed1((uint64_t)high_word << 32 | low_word);
    init_power_table_mod_prime1(cache->rng_table, rng_seed);
}

static void ethash_compute_full_node(
        node *const ret,
        const unsigned node_index,
        ethash_params const *params,
        node const *nodes,
        uint32_t const *rng_table
) {
    size_t num_parent_nodes = params->cache_size / sizeof(node);
    assert(node_index >= num_parent_nodes);

	memset(ret, 0, sizeof(*ret));
    
    uint32_t rand = quick_bbs(rng_table, node_index * 2);
    for (unsigned i = 0; i != params->k; ++i) {

        size_t parent_index = rand % num_parent_nodes;
        rand = cube_mod_safe_prime2(rand);

		node const* parent = &nodes[parent_index];
        for (unsigned w = 0; w != NODE_WORDS; ++w) {
            ret->words[w] = fnv_hash(ret->words[w], parent->words[w]);
        }
    }
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
        const uint8_t prevhash[32],
        const uint64_t nonce) {
    // for simplicity the cache and dag must be whole number of pages
    assert((params->cache_size % PAGE_WORDS) == 0);
    assert((params->full_size % PAGE_WORDS) == 0);

    // pack seed and nonce together
    struct {
        uint8_t seed[32];
        uint64_t nonce;
    } init;
    memcpy(init.seed, prevhash, 32);
    init.nonce = fix_endian64(nonce);

    // compute sha3-256 hash and replicate across mix
    uint32_t mix[PAGE_WORDS];
    SHA3_256((uint8_t*)mix, (uint8_t const *) &init, sizeof(init));
#if BYTE_ORDER != LITTLE_ENDIAN
	for (unsigned w = 0; w != 8; ++w) {
		mix[w] = fix_endian32(mix[w]);
	}
#endif
    for (unsigned w = 8; w != PAGE_WORDS; ++w) {
        mix[w] = mix[w % 8];
    }

	// todo, do we need 64-bits?
    uint32_t const low_word = mix[0];
	uint32_t const high_word = mix[1];
    uint32_t rand = make_seed1((uint64_t)high_word << 32 | low_word);

    unsigned const
            page_size = sizeof(uint32_t) * PAGE_WORDS,
            page_reads = params->hash_read_size / page_size,
            num_full_pages = params->full_size / page_size,
            num_cache_pages = params->cache_size / page_size,
            num_pages = rng_table ? num_cache_pages : num_full_pages;

    for (unsigned i = 0; i != page_reads; ++i) {
        unsigned const index = rand % num_full_pages;
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

        for (unsigned w = 0; w != PAGE_WORDS; ++w) {
            mix[w] = fnv_hash(mix[w], page->words[w]);
        }
    }

#if BYTE_ORDER != LITTLE_ENDIAN
	for (unsigned w = 0; w != PAGE_WORDS; ++w) {
		mix[w] = fix_endian32(mix[w]);
	}
#endif

    uint8_t tmp[32];
    SHA3_256((uint8_t *const) &tmp, (uint8_t const *) mix, sizeof(mix));
    SHA3_256(ret, (uint8_t const *) &tmp, sizeof(tmp));
}

void ethash_full(uint8_t ret[32], void const *full_mem, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce) {
    // todo: Maybe not use a null pointer here?
    ethash_hash(ret, (node const *) full_mem, NULL, params, previous_hash, nonce);
}

void ethash_light(uint8_t ret[32], ethash_cache const *cache, ethash_params const *params, const uint8_t previous_hash[32], const uint64_t nonce) {
    ethash_hash(ret, (node const *) cache->mem, cache->rng_table, params, previous_hash, nonce);
}
