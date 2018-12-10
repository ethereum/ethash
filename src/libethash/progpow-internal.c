/*
  This file is part of ethash.

  ethash is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ethash is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ethash.	If not, see <http://www.gnu.org/licenses/>.
*/
/** @file progpow-internal.c
 * @license CC0
 * @author ifdefelse <ifdefelse@protonmail.com>
 * @date 2018
 */

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <errno.h>
#include <math.h>
#include "mmap.h"
#include "ethash.h"
#include "fnv.h"
#include "endian.h"
#include "internal.h"
#include "io.h"

#ifdef WITH_CRYPTOPP

#include "sha3_cryptopp.h"

#else
#include "sha3.h"
#endif // WITH_CRYPTOPP

#define PROGPOW_LANES                   16
#define PROGPOW_REGS                    32
#define PROGPOW_DAG_LOADS                4
#define PROGPOW_CACHE_BYTES             (16*1024)
#define PROGPOW_CNT_DAG                 ETHASH_ACCESSES
#define PROGPOW_CNT_CACHE               12
#define PROGPOW_CNT_MATH                20
#define PROGPOW_CACHE_WORDS  (PROGPOW_CACHE_BYTES / sizeof(uint32_t))
#define PROGPOW_PERIOD                  50

#define ROTL(x,n,w) (((x) << (n % w)) | ((x) >> ((w) - (n % w))))
#define ROTL32(x,n) ROTL(x,n,32)	/* 32 bits word */

#define ROTR(x,n,w) (((x) >> (n % w)) | ((x) << ((w) - (n % w))))
#define ROTR32(x,n) ROTR(x,n,32)	/* 32 bits word */

#define min(a,b) ((a<b) ? a : b)
//#define mul_hi32(a, b) __umulhi(a, b)
static inline uint32_t mul_hi32(uint32_t a, uint32_t b)
{
	uint64_t result = (uint64_t) a * (uint64_t) b;
	return  (uint32_t) (result>>32);
}

#ifdef __GNUC__
#define clz(a) (a ? (uint32_t)__builtin_clz(a) : 32)
#define popcount(a) ((uint32_t)__builtin_popcount(a))
#elif _MSC_VER
#include <intrin.h>
static inline uint32_t clz(uint32_t value)
{
	unsigned long leading_zero = 0;

	if (_BitScanReverse(&leading_zero, value)) {
		return 31 - leading_zero;
	} else {
		// Same remarks as above
		return 32;
	}
}

static inline uint32_t popcount(uint32_t value)
{
    return (uint32_t)__popcnt(value);
}
#else
static inline uint32_t clz(uint32_t a)
{
	uint32_t result = 0;
	for (int i = 31; i >= 0; i--) {
		if (((a>>i)&1) == 0)
			result ++;
		else
			break;
	}
	return result;
}

static inline uint32_t popcount(uint32_t a)
{
	uint32_t result = 0;
	for (int i = 31; i >= 0; i--) {
		if(((a>>i)&1) == 1)
			result ++;
	}
	return result;
}
#endif

static inline void swap(uint32_t *a, uint32_t *b)
{
	uint32_t t = *a;
	*a = *b;
	*b = t;
}

static inline uint32_t fnv1a(uint32_t *h, uint32_t d)
{
        return *h = (*h ^ d) * (uint32_t)0x1000193;
}

// Implementation based on:
// https://github.com/mjosaarinen/tiny_sha3/blob/master/sha3.c
const uint32_t keccakf_rndc[24] = {
	0x00000001, 0x00008082, 0x0000808a, 0x80008000, 0x0000808b, 0x80000001,
	0x80008081, 0x00008009, 0x0000008a, 0x00000088, 0x80008009, 0x8000000a,
	0x8000808b, 0x0000008b, 0x00008089, 0x00008003, 0x00008002, 0x00000080,
	0x0000800a, 0x8000000a, 0x80008081, 0x00008080, 0x80000001, 0x80008008
};

// Implementation of the permutation Keccakf with width 800.
void keccak_f800_round(uint32_t st[25], const int r)
{
	const uint32_t keccakf_rotc[24] = {
		1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
		27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
	};
	const uint32_t keccakf_piln[24] = {
		10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
		15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
	};

	uint32_t t, bc[5];
	// Theta
	for (int i = 0; i < 5; i++)
		bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

	for (int i = 0; i < 5; i++) {
		t = bc[(i + 4) % 5] ^ ROTL32(bc[(i + 1) % 5], 1);
		for (uint32_t j = 0; j < 25; j += 5)
			st[j + i] ^= t;
	}

	// Rho Pi
	t = st[1];
	for (int i = 0; i < 24; i++) {
		uint32_t j = keccakf_piln[i];
		bc[0] = st[j];
		st[j] = ROTL32(t, keccakf_rotc[i]);
		t = bc[0];
	}

	//  Chi
	for (uint32_t j = 0; j < 25; j += 5) {
		for (int i = 0; i < 5; i++)
			bc[i] = st[j + i];
		for (int i = 0; i < 5; i++)
			st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
	}

	//  Iota
	st[0] ^= keccakf_rndc[r];
}

// Implementation of the Keccak sponge construction (with padding omitted)
// The width is 800, with a bitrate of 576, and a capacity of 224.
hash32_t keccak_f800_progpow(hash32_t header, uint64_t seed, hash32_t digest)
{
	uint32_t st[25];

	for (int i = 0; i < 25; i++)
		st[i] = 0;
	for (int i = 0; i < 8; i++)
		st[i] = header.uint32s[i];
	st[8] = seed;
	st[9] = seed >> 32;
	for (int i = 0; i < 8; i++)
		st[10+i] = digest.uint32s[i];

	for (int r = 0; r < 21; r++) {
		keccak_f800_round(st, r);
	}
	// last round can be simplified due to partial output
	keccak_f800_round(st, 21);

	hash32_t ret;
	for (int i = 0; i < 8; i++) {
		ret.uint32s[i] = st[i];
	}

	return ret;
}

typedef struct {
	uint32_t z, w, jsr, jcong;
} kiss99_t;

// KISS99 is simple, fast, and passes the TestU01 suite
// https://en.wikipedia.org/wiki/KISS_(algorithm)
// http://www.cse.yorku.ca/~oz/marsaglia-rng.html
uint32_t kiss99(kiss99_t * st)
{
	st->z = 36969 * (st->z & 65535) + (st->z >> 16);
	st->w = 18000 * (st->w & 65535) + (st->w >> 16);
	uint32_t MWC = ((st->z << 16) + st->w);
	st->jsr ^= (st->jsr << 17);
	st->jsr ^= (st->jsr >> 13);
	st->jsr ^= (st->jsr << 5);
	st->jcong = 69069 * st->jcong + 1234567;
	return ((MWC^st->jcong) + st->jsr);
}

void fill_mix(
    uint64_t seed,
    uint32_t lane_id,
    uint32_t mix[PROGPOW_REGS]
)
{
	// Use FNV to expand the per-warp seed to per-lane
	// Use KISS to expand the per-lane seed to fill mix
	uint32_t fnv_hash = 0x811c9dc5;
	kiss99_t st;
	st.z = fnv1a(&fnv_hash, (uint32_t)seed);
	st.w = fnv1a(&fnv_hash, seed >> 32);
	st.jsr = fnv1a(&fnv_hash, lane_id);
	st.jcong = fnv1a(&fnv_hash, lane_id);
	for (int i = 0; i < PROGPOW_REGS; i++)
		mix[i] = kiss99(&st);
}

kiss99_t progPowInit(uint64_t prog_seed, uint32_t mix_seq_dst[PROGPOW_REGS], uint32_t mix_seq_src[PROGPOW_REGS])
{
	kiss99_t prog_rnd;
	uint32_t fnv_hash = 0x811c9dc5;
	prog_rnd.z = fnv1a(&fnv_hash, (uint32_t)prog_seed);
	prog_rnd.w = fnv1a(&fnv_hash, prog_seed >> 32);
	prog_rnd.jsr = fnv1a(&fnv_hash, (uint32_t)prog_seed);
	prog_rnd.jcong = fnv1a(&fnv_hash, prog_seed >> 32);
	// Create a random sequence of mix destinations for merge() and mix sources for cache reads
	// guaranteeing every location is touched once
	// guarantees no duplicate cache reads, which could be optimized away
	// Uses Fisher–Yates shuffle
	for (int i = 0; i < PROGPOW_REGS; i++)
	{
		mix_seq_dst[i] = i;
		mix_seq_src[i] = i;
	}
	for (int i = PROGPOW_REGS - 1; i > 0; i--)
	{
		int j;
		j = kiss99(&prog_rnd) % (i + 1);
		swap(&(mix_seq_dst[i]), &(mix_seq_dst[j]));
		j = kiss99(&prog_rnd) % (i + 1);
		swap(&(mix_seq_src[i]), &(mix_seq_src[j]));
	}
	return prog_rnd;
}

// Merge new data from b into the value in a
// Assuming A has high entropy only do ops that retain entropy
// even if B is low entropy
// (IE don't do A&B)
void merge(uint32_t *a, uint32_t b, uint32_t r)
{
	switch (r % 4)
	{
		case 0: *a = (*a * 33) + b; break;
		case 1: *a = (*a ^ b) * 33; break;
		// prevent rotate by 0 which is a NOP
		case 2: *a = ROTL32(*a, ((r >> 16) % 31)+1) ^ b; break;
		case 3: *a = ROTR32(*a, ((r >> 16) % 31)+1) ^ b; break;
	}
}

// Random math between two input values
uint32_t progpowMath(uint32_t a, uint32_t b, uint32_t r)
{
	switch (r % 11)
	{
		default:
		case 0: return a + b; break;
		case 1: return a * b; break;
		case 2: return mul_hi32(a, b); break;
		case 3: return min(a, b); break;
		case 4: return ROTL32(a, b); break;
		case 5: return ROTR32(a, b); break;
		case 6: return a & b; break;
		case 7: return a | b; break;
		case 8: return a ^ b; break;
		case 9: return clz(a) + clz(b); break;
		case 10: return popcount(a) + popcount(b); break;
	}
	return 0;
}

void progPowLoop(
	const uint64_t prog_seed,
	const uint32_t loop,
	ethash_light_t const light,
	uint32_t mix[PROGPOW_LANES][PROGPOW_REGS],
	const uint32_t *g_dag,
	const uint32_t *c_dag,
	const uint32_t dag_words)
{
	// All lanes share a base address for the global load
	// Global offset uses mix[0] to guarantee it depends on the load result
	uint32_t offset_g = mix[loop%PROGPOW_LANES][0] % (64 * dag_words / (PROGPOW_LANES*PROGPOW_DAG_LOADS));

	// global load to sequential locations
	uint32_t data_g[PROGPOW_LANES][PROGPOW_DAG_LOADS];
	uint32_t dag_data[PROGPOW_LANES*PROGPOW_DAG_LOADS];
	if (g_dag) {
		for (int i = 0; i < PROGPOW_DAG_LOADS; i++) {
			memcpy((void *)&dag_data[PROGPOW_LANES*i],
				(void *)&g_dag[offset_g*PROGPOW_LANES*PROGPOW_DAG_LOADS + i*PROGPOW_LANES],
				PROGPOW_LANES*sizeof(uint32_t));
		}
	} else {
		node tmp_node;
		for (int i = 0; i < PROGPOW_DAG_LOADS; i++) {
			uint64_t k = offset_g*PROGPOW_LANES*PROGPOW_DAG_LOADS + i*PROGPOW_LANES;
			ethash_calculate_dag_item(&tmp_node, k / 16, light);
			memcpy((void *)&dag_data[PROGPOW_LANES*i],
				(void *)&tmp_node.words[0],
				PROGPOW_LANES*sizeof(uint32_t));
		}
	}

	//int max_i = max(PROGPOW_CNT_CACHE, PROGPOW_CNT_MATH);
	int max_i;
	if (PROGPOW_CNT_CACHE > PROGPOW_CNT_MATH)
		max_i = PROGPOW_CNT_CACHE;
	else
		max_i = PROGPOW_CNT_MATH;

	// Initialize the program seed and sequences
	// When mining these are evaluated on the CPU and compiled away
	uint32_t mix_seq_dst[PROGPOW_REGS];
	uint32_t mix_seq_src[PROGPOW_REGS];
	uint32_t mix_seq_dst_cnt = 0;
	uint32_t mix_seq_src_cnt = 0;
	kiss99_t prog_rnd = progPowInit(prog_seed, mix_seq_dst, mix_seq_src);

	for (int i = 0; i < max_i; i++)
	{
		if (i < PROGPOW_CNT_CACHE)
		{
			// Cached memory access
			// lanes access random 32-bit locations within the first portion of the DAG
			int src = mix_seq_src[(mix_seq_src_cnt++)%PROGPOW_REGS];
			int dst = mix_seq_dst[(mix_seq_dst_cnt++)%PROGPOW_REGS];
			int sel = kiss99(&prog_rnd);
			for (int l = 0; l < PROGPOW_LANES; l++)
			{
				uint32_t offset = mix[l][src] % PROGPOW_CACHE_WORDS;
				merge(&(mix[l][dst]), c_dag[offset], sel);
			}
		}
		if (i < PROGPOW_CNT_MATH)
		{
			// Random Math
			// Generate 2 unique sources
			int src_rnd = kiss99(&prog_rnd) % (PROGPOW_REGS * (PROGPOW_REGS-1));
			int src1 = src_rnd % PROGPOW_REGS; // 0 <= src1 < PROGPOW_REGS
			int src2 = src_rnd / PROGPOW_REGS; // 0 <= src2 < PROGPOW_REGS - 1
			if (src2 >= src1) ++src2; // src2 is now any reg other than src1
			int sel1 = kiss99(&prog_rnd);
			int dst = mix_seq_dst[(mix_seq_dst_cnt++)%PROGPOW_REGS];
			int sel2 = kiss99(&prog_rnd);
			for (int l = 0; l < PROGPOW_LANES; l++)
			{
				uint32_t data = progpowMath(mix[l][src1], mix[l][src2], sel1);
				merge(&(mix[l][dst]), data, sel2);
			}
		}
	}
	for (int l = 0; l < PROGPOW_LANES; l++)
	{
		// global load to the 256 byte DAG entry
		// every lane can access every part of the entry
		uint32_t index = ((l ^ loop) % PROGPOW_LANES) * PROGPOW_DAG_LOADS;
		for (int i = 0; i < PROGPOW_DAG_LOADS; i++)
			data_g[l][i] = dag_data[index+i];
	}

	// Consume the global load data at the very end of the loop to allow full latency hiding
	// Always merge into mix[0] to feed the offset calculation
	for (int i = 0; i < PROGPOW_DAG_LOADS; i++)
	{
		int dst = (i==0) ? 0 : mix_seq_dst[(mix_seq_dst_cnt++)%PROGPOW_REGS];
		int sel = kiss99(&prog_rnd);
		for (int l = 0; l < PROGPOW_LANES; l++)
			merge(&(mix[l][dst]), data_g[l][i], sel);
	}
}

static bool progpow_hash(
	ethash_return_value_t* ret,
	node const* full_nodes,
	ethash_light_t const light,
	uint64_t full_size,
	ethash_h256_t const header_hash,
	uint64_t const nonce,
	uint64_t const block_number
)
{
	uint32_t *g_dag = NULL;

	const hash32_t header;
	memcpy((void *)&header, (void *)&header_hash, sizeof(header_hash));
	uint32_t c_dag[PROGPOW_CACHE_WORDS];
	if (full_nodes) {
		g_dag = (uint32_t *) full_nodes;
		for(int l = 0; l < PROGPOW_LANES; l++)
		{
			// Load random data into the cache
			// TODO: should be a new blob of data, not existing DAG data
			for (uint32_t word = l*4; word < PROGPOW_CACHE_WORDS; word += PROGPOW_LANES*4)
			{
				c_dag[word + 0] = g_dag[word + 0];
				c_dag[word + 1] = g_dag[word + 1];
				c_dag[word + 2] = g_dag[word + 2];
				c_dag[word + 3] = g_dag[word + 3];
			}
		}
	} else {
		node tmp_node;
		for(int l = 0; l < PROGPOW_LANES; l++)
		{
			for (uint32_t word = l*NODE_WORDS; word < PROGPOW_CACHE_WORDS; word += PROGPOW_LANES*NODE_WORDS)
			{
				ethash_calculate_dag_item(&tmp_node, word / NODE_WORDS, light);
				memcpy((void *)&c_dag[word], (void *)&tmp_node.words[0], sizeof(uint32_t) * NODE_WORDS);
			}
		}
	}

	uint32_t mix[PROGPOW_LANES][PROGPOW_REGS];
	hash32_t digest;
	for (int i = 0; i < 8; i++)
		digest.uint32s[i] = 0;

	// keccak(header..nonce)
	hash32_t seed_256 = keccak_f800_progpow(header, nonce, digest);
	// endian swap so byte 0 of the hash is the MSB of the value
	uint64_t seed = (uint64_t)ethash_swap_u32(seed_256.uint32s[0]) << 32 | ethash_swap_u32(seed_256.uint32s[1]);

	// initialize mix for all lanes
	for (int l = 0; l < PROGPOW_LANES; l++)
		fill_mix(seed, l, mix[l]);

	uint64_t prog_seed = block_number / PROGPOW_PERIOD;
	uint32_t dagWords = (unsigned)((uint32_t)full_size / PROGPOW_MIX_BYTES);
	// execute the randomly generated inner loop
	for (int i = 0; i < PROGPOW_CNT_DAG; i++)
	{
		progPowLoop(prog_seed, i, light, mix, g_dag, c_dag, dagWords);
	}

	// Reduce mix data to a single per-lane result
	uint32_t lane_hash[PROGPOW_LANES];
	for (int l = 0; l < PROGPOW_LANES; l++)
	{
		lane_hash[l] = 0x811c9dc5;
		for (int i = 0; i < PROGPOW_REGS; i++)
			fnv1a(&lane_hash[l], mix[l][i]);
	}
	// Reduce all lanes to a single 256-bit result
	for (int i = 0; i < 8; i++)
		digest.uint32s[i] = 0x811c9dc5;

	for (int l = 0; l < PROGPOW_LANES; l++)
		fnv1a(&digest.uint32s[l%8], lane_hash[l]);

	memset((void *)&ret->mix_hash, 0, sizeof(ret->mix_hash));
	memcpy(&ret->mix_hash, (void *)&digest, sizeof(digest));
	memset((void *)&ret->result, 0, sizeof(ret->result));
	digest = keccak_f800_progpow(header, seed, digest);
	memcpy((void *)&ret->result, (void *)&digest, sizeof(ret->result));

	return true;
}

ethash_return_value_t progpow_light_compute_internal(
	ethash_light_t light,
	uint64_t full_size,
	ethash_h256_t const header_hash,
	uint64_t nonce,
	uint64_t block_number
)
{
	ethash_return_value_t ret;
	ret.success = true;
	if (!progpow_hash(&ret, NULL, light, full_size, header_hash, nonce, block_number)) {
		ret.success = false;
	}
	return ret;
}

ethash_return_value_t progpow_light_compute(
	ethash_light_t light,
	ethash_h256_t const header_hash,
	uint64_t nonce,
	uint64_t block_number
)
{
	uint64_t full_size = ethash_get_datasize(block_number);
	return progpow_light_compute_internal(light, full_size, header_hash, nonce, block_number);
}

ethash_return_value_t progpow_full_compute(
	ethash_full_t full,
	ethash_h256_t const header_hash,
	uint64_t nonce,
	uint64_t block_number
)
{
	ethash_return_value_t ret;
	ret.success = true;
	if (!progpow_hash(
		&ret,
		(node const*)full->data,
		NULL,
		full->file_size,
		header_hash,
		nonce,
		block_number)) {
		ret.success = false;
	}
	return ret;
}
