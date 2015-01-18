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
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "dash.h"

#ifdef _MSC_VER

// foward declare without all of Windows.h
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);

static void logf(const char *str, ...)
{
	va_list args;
    va_start(args, str);

	char buf[4096];
	vsnprintf(buf, sizeof(buf), str, args);
	buf[sizeof(buf)-1] = '\0';
	OutputDebugStringA(buf);
}

#else
#define logf printf
#endif

// forward declare
static inline void sha3_256(void* ret, void const* data, size_t size);
static inline void sha3_512(void* ret, void const* data, size_t size);


#define SAFE_PRIME 4294967087U
#define SAFE_PRIME_TOTIENT 4294967086U
#define SAFE_PRIME_TOTIENT_TOTIENT 2147483542
#define SAFE_PRIME2 4294965887U

static inline uint32_t cube_mod_safe_prime(const uint32_t x) {
    uint64_t temp = x;
    temp *= x;
    temp %= SAFE_PRIME;
    temp *= x;
    return (uint32_t) (temp % SAFE_PRIME);
}

static inline uint32_t cube_mod_safe_prime2(const uint32_t x) {
    uint64_t temp = x;
    temp *= x;
    temp %= SAFE_PRIME2;
    temp *= x;
    return (uint32_t) (temp % SAFE_PRIME2);
}

static const uint32_t powers_of_three_mod_totient[32] = {
        3, 9, 81, 6561, 43046721, 3884235087U, 4077029855U, 106110483U,
        2292893763U, 2673619771U, 2265535291U, 2641962139U, 867632699U,
        4234161123U, 4065670495U, 1161610561U, 1960994291U, 683176121U,
        1539788995U, 1214448689U, 2554812497U, 2574646649U, 3290602031U,
        2381552417U, 3391635653U, 639421717U, 1685119297U, 4206074945U,
        1006214457U, 102532655U, 4081098815U, 3106101787U
};

uint32_t three_pow_mod_totient(uint32_t p) {
    uint64_t r = 1;
    for(int i = 0; i < 32; ++i) {
        if (p & 1) {
            r *= powers_of_three_mod_totient[i];
            r %= SAFE_PRIME_TOTIENT;
        }
        p >>= 1;
    }
    return (uint32_t) r;
}

void init_power_table_mod_prime(uint32_t table[32], const uint32_t n) {
    uint64_t r = n;
    r %= SAFE_PRIME;
    table[0] = (uint32_t) r;
    for(int i = 1; i < 32; ++i) {
        r *= r;
        r %= SAFE_PRIME;
        table[i] = (uint32_t) r;
    }
}

uint32_t quick_bbs(const uint32_t power_table[32], const uint64_t p) {
    uint32_t q = three_pow_mod_totient(
            (uint32_t) (p % SAFE_PRIME_TOTIENT_TOTIENT));
    uint64_t r = 1;
    for(int i = 0; i < 32; ++i) {
        if (q & 1) {
            r *= power_table[i];
            r %= SAFE_PRIME;
        }
        q >>= 1;
    }
    return (uint32_t) r;
}

static inline uint32_t min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

static inline uint32_t clamp_u32(uint32_t x, uint32_t min_, uint32_t max_)
{
	return x < min_ ? min_ : (x > max_ ? max_ : x);
}

static inline uint32_t make_seed(uint64_t seed, uint32_t safe_prime)
{
	return clamp_u32((uint32_t)(seed % safe_prime), 2, safe_prime-2);
}

void compute_cache_nodes_placeholder(node* ret, parameters const* params)
{
	assert((params->cache_size % sizeof(node)) == 0);

	sha3_512(ret[0].bytes, &params->seed, 32);

	for (unsigned i = 1; i < params->cache_size/sizeof(node); ++i)
	{
		sha3_512(ret[i].bytes, ret[i-1].bytes, 64);
	}
}

void compute_full_node(
	node* ret,
	node const* nodes,
	unsigned node_index,
	parameters const* params,
	uint32_t const rng_table[32]
	)
{
	uint32_t const num_parent_nodes = params->cache_size/sizeof(node);
	assert(node_index >= num_parent_nodes);

	uint32_t picker1 = quick_bbs(rng_table, node_index*2);
	uint32_t picker2 = cube_mod_safe_prime(picker1);

	for (unsigned w = 0; w != NODE_WORDS; ++w)
	{
		// todo, endian
		ret->words[w] = (uint64_t)picker1 << 32 | picker2; //todo, better than this?
	}

	uint32_t rand = make_seed(ret->words[0], SAFE_PRIME2);
	for (unsigned i = 0; i != params->k; ++i)
	{
		// todo, endian
		uint32_t parent_index = (uint32_t)(ret->words[i % NODE_WORDS] % num_parent_nodes);
		if (USE_RNG_FOR_NODES)
		{
			parent_index = rand % num_parent_nodes;
			rand = cube_mod_safe_prime2(rand);
		}

		for (unsigned w = 0; w != NODE_WORDS; ++w)
		{
			ret->words[w] ^= nodes[parent_index].words[w];
		}
	}
}


void compute_full_nodes(node* ret, parameters const* params)
{
	assert((params->full_size % (sizeof(uint64_t)*PAGE_WORDS)) == 0);
	assert((params->full_size % sizeof(node)) == 0);

	// compute cache nodes first
	compute_cache_nodes_placeholder(ret, params);

	// todo, endian
	uint32_t rng_seed = make_seed(ret->words[0], SAFE_PRIME);
    uint32_t rng_table[32];
    init_power_table_mod_prime(rng_table, rng_seed);

	// now compute full nodes
	for (unsigned n = params->cache_size/sizeof(node); n != params->full_size/sizeof(node); ++n)
	{
		compute_full_node(&ret[n], ret, n, params, rng_table);
	}
}

void compute_hash(h256* ret, node const* nodes, parameters const* params, uint64_t nonce, bool is_full)
{
	// for simplicity the cache and dag must be whole number of pages
	assert((params->cache_size % PAGE_WORDS) == 0);
	assert((params->full_size % PAGE_WORDS) == 0);
	
	// pack seed and nonce together, todo, endian
	struct { h256 seed; uint64_t nonce; } init = { params->seed, nonce };

	// compute sha3-256 hash and replicate across mix
	uint64_t mix[PAGE_WORDS];
	sha3_256(mix, &init, sizeof(init));
	for (unsigned w = 4; w != PAGE_WORDS; ++w)
	{
		mix[w] = mix[w & 3];
	}

	// todo, endian
	uint32_t rng_seed;
    uint32_t rng_table[32];
	if (!is_full)
	{
		// todo, this can be done once
		rng_seed = make_seed(nodes->words[0], SAFE_PRIME);
		init_power_table_mod_prime(rng_table, rng_seed);
	}

	// seed for RNG variant
	uint32_t rand = (uint32_t)mix[0];

	unsigned const page_size = sizeof(uint64_t) * PAGE_WORDS;
	unsigned num_full_pages = params->full_size / page_size;
	unsigned num_pages = is_full ? num_full_pages : params->cache_size / page_size;

	for (unsigned i = 0; i != params->page_accesses; ++i)
	{
		// most architectures are little-endian now, we can speed things up
		// by specifying the byte order as little endian for this part.
		uint64_t mixi = mix[i % PAGE_WORDS];

		unsigned index = (unsigned)(mixi % num_full_pages);
		if (USE_RNG_FOR_HASH)
		{
			index = rand % num_full_pages;
			rand = cube_mod_safe_prime(rand);
		}

		node tmp_page[PAGE_NODES];
		node const* page;
		if (index >= num_pages)
		{
			page = tmp_page;

			for (unsigned n = 0; n != PAGE_NODES; ++n)
			{
				compute_full_node(&tmp_page[n], nodes, index*PAGE_NODES + n, params, rng_table);
			}
		}
		else
		{
			page = nodes + PAGE_NODES*index;
		}

		for (unsigned w = 0; w != PAGE_WORDS; ++w)
		{
			mix[w] ^= page->words[w];
		}
	}

	h256 tmp;
	sha3_256(&tmp, mix, sizeof(mix));
	sha3_256(ret, &tmp, sizeof(tmp));
}

#include <libethcore/CryptoHeaders.h>
#include <libdevcore/Common.h>
#include "libdevcore/FixedHash.h"

static inline void sha3_256(void* ret, void const* data, size_t size)
{
	CryptoPP::SHA3_256().CalculateDigest((byte*)ret, (byte*)data, size);
}

static inline void sha3_512(void* ret, void const* data, size_t size)
{
	CryptoPP::SHA3_512().CalculateDigest((byte*)ret, (byte*)data, size);
}

namespace dev
{

::h256 g_hash;

extern "C" void main(void)
{
	// make random seed and header
	srand(3248723843);
	h256 seed = 0;
	h256 header = 0;
	for (unsigned i = 0; i != 256; i += 16)
	{
		seed = (u256)seed << 16 | rand();
		header = (u256)header << 16 | rand();
	}

	const unsigned size = 262147 * 4096;
	//const unsigned size = 32771 * 4096;
	const unsigned trials = 1000;
	const bool test_full = false;

	parameters params = { 0 };
	params.full_size = size;
	params.cache_size = 8209*4096;
	params.k = 2 * (params.full_size / params.cache_size);
	memcpy(&params.seed, &seed, sizeof(seed));

	// allocate page aligned buffer for dag
	void* nodes_buf = malloc(size + 4095 + 64);
	node* nodes = (node*)((uintptr_t(nodes_buf) + 4095) & ~4095);
	{
		clock_t startTime = clock();

		if (test_full)
			compute_full_nodes(nodes, &params);
		else
			compute_cache_nodes_placeholder(nodes, &params);

		clock_t time = clock() - startTime;
		logf("compute_nodes: %ums\n", time);
	}

	for (unsigned accesses = 1; accesses <= 4096; accesses <<= 1)
	{
		params.page_accesses = accesses;
		
		clock_t startTime = clock();

		// uncomment to enable multicore
		//#pragma omp parallel for
		for (int i = 0; i < trials; ++i)
		{
			compute_hash(&g_hash, nodes, &params, i, test_full);
			//if ((i % 500) == 0)
			//	logf("trials: %u\n", i);
		}
		clock_t time = clock() - startTime;

		logf("accesses %5u, hashrate: %5u, bw: %5u MB/s\n", accesses, (trials*1000)/time, (unsigned)((((uint64_t)trials*accesses*PAGE_WORDS*8*1000)/time) / (1024*1024)));
	}

	free(nodes_buf);
}

}
