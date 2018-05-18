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
#include "data_sizes.h"
#include "io.h"

#ifdef WITH_CRYPTOPP

#include "sha3_cryptopp.h"

#else
#include "sha3.h"
#endif // WITH_CRYPTOPP

uint64_t ethash_get_datasize(uint64_t const block_number)
{
	assert(block_number / ETHASH_EPOCH_LENGTH < 2048);
	return dag_sizes[block_number / ETHASH_EPOCH_LENGTH];
}

uint64_t ethash_get_cachesize(uint64_t const block_number)
{
	assert(block_number / ETHASH_EPOCH_LENGTH < 2048);
	return cache_sizes[block_number / ETHASH_EPOCH_LENGTH];
}

// Follows Sergio's "STRICT MEMORY HARD HASHING FUNCTIONS" (2014)
// https://bitslog.files.wordpress.com/2013/12/memohash-v0-3.pdf
// SeqMemoHash(s, R, N)
static bool ethash_compute_cache_nodes(
	node* const nodes,
	uint64_t cache_size,
	ethash_h256_t const* seed
)
{
	if (cache_size % sizeof(node) != 0) {
		return false;
	}
	uint32_t const num_nodes = (uint32_t) (cache_size / sizeof(node));

	SHA3_512(nodes[0].bytes, (uint8_t*)seed, 32);

	for (uint32_t i = 1; i != num_nodes; ++i) {
		SHA3_512(nodes[i].bytes, nodes[i - 1].bytes, 64);
	}

	for (uint32_t j = 0; j != ETHASH_CACHE_ROUNDS; j++) {
		for (uint32_t i = 0; i != num_nodes; i++) {
			uint32_t const idx = nodes[i].words[0] % num_nodes;
			node data;
			data = nodes[(num_nodes - 1 + i) % num_nodes];
			for (uint32_t w = 0; w != NODE_WORDS; ++w) {
				data.words[w] ^= nodes[idx].words[w];
			}
			SHA3_512(nodes[i].bytes, data.bytes, sizeof(data));
		}
	}

	// now perform endian conversion
	fix_endian_arr32(nodes->words, num_nodes * NODE_WORDS);
	return true;
}

void ethash_calculate_dag_item(
	node* const ret,
	uint32_t node_index,
	ethash_light_t const light
)
{
	uint32_t num_parent_nodes = (uint32_t) (light->cache_size / sizeof(node));
	node const* cache_nodes = (node const *) light->cache;
	node const* init = &cache_nodes[node_index % num_parent_nodes];
	memcpy(ret, init, sizeof(node));
	ret->words[0] ^= node_index;
	SHA3_512(ret->bytes, ret->bytes, sizeof(node));
#if defined(_M_X64) && ENABLE_SSE
	__m128i const fnv_prime = _mm_set1_epi32(FNV_PRIME);
	__m128i xmm0 = ret->xmm[0];
	__m128i xmm1 = ret->xmm[1];
	__m128i xmm2 = ret->xmm[2];
	__m128i xmm3 = ret->xmm[3];
#elif defined(__MIC__)
	__m512i const fnv_prime = _mm512_set1_epi32(FNV_PRIME);
	__m512i zmm0 = ret->zmm[0];
#endif

	for (uint32_t i = 0; i != ETHASH_DATASET_PARENTS; ++i) {
		uint32_t parent_index = fnv_hash(node_index ^ i, ret->words[i % NODE_WORDS]) % num_parent_nodes;
		node const *parent = &cache_nodes[parent_index];

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

			// have to write to ret as values are used to compute index
			ret->xmm[0] = xmm0;
			ret->xmm[1] = xmm1;
			ret->xmm[2] = xmm2;
			ret->xmm[3] = xmm3;
		}
		#elif defined(__MIC__)
		{
			zmm0 = _mm512_mullo_epi32(zmm0, fnv_prime);

			// have to write to ret as values are used to compute index
			zmm0 = _mm512_xor_si512(zmm0, parent->zmm[0]);
			ret->zmm[0] = zmm0;
		}
		#else
		{
			for (unsigned w = 0; w != NODE_WORDS; ++w) {
				ret->words[w] = fnv_hash(ret->words[w], parent->words[w]);
			}
		}
#endif
	}
	SHA3_512(ret->bytes, ret->bytes, sizeof(node));
}

bool ethash_compute_full_data(
	void* mem,
	uint64_t full_size,
	ethash_light_t const light,
	ethash_callback_t callback
)
{
	if (full_size % (sizeof(uint32_t) * MIX_WORDS) != 0 ||
		(full_size % sizeof(node)) != 0) {
		return false;
	}
	uint32_t const max_n = (uint32_t)(full_size / sizeof(node));
	node* full_nodes = mem;
	double const progress_change = 1.0f / max_n;
	double progress = 0.0f;
	// now compute full nodes
	for (uint32_t n = 0; n != max_n; ++n) {
		if (callback &&
			n % (max_n / 100) == 0 &&
			callback((unsigned int)(ceil(progress * 100.0f))) != 0) {

			return false;
		}
		progress += progress_change;
		ethash_calculate_dag_item(&(full_nodes[n]), n, light);
	}
	return true;
}

//***************************************************************
//***************************************************************
typedef struct
{
	uint32_t uint32s[32 / sizeof(uint32_t)];
} hash32_t;

#define PROGPOW_LANES                   32
#define PROGPOW_REGS                    16
#define PROGPOW_CACHE_BYTES             (16*1024)
#define PROGPOW_CNT_MEM                 ETHASH_ACCESSES
#define PROGPOW_CNT_CACHE               8
#define PROGPOW_CNT_MATH                8
#define PROGPOW_CACHE_WORDS  (PROGPOW_CACHE_BYTES / sizeof(uint32_t))

//#define ROTL32(x,n) __funnelshift_l((x), (x), (n))
//#define ROTR32(x,n) __funnelshift_r((x), (x), (n))
#define ROTL(x,n,w) (((x) << (n)) | ((x) >> ((w) - (n))))
#define ROTL32(x,n) ROTL(x,n,32)	/* 32 bits word */

#define ROTR(x,n,w) (((x) >> (n)) | ((x) << ((w) - (n))))
#define ROTR32(x,n) ROTR(x,n,32)	/* 32 bits word */
//#define ROTR32(x, n)  (((0U + (x)) << (32 - (n))) | ((x) >> (n)))  // Assumes that x is uint32_t and 0 < n < 32


#define min(a,b) ((a<b) ? a : b)
//#define mul_hi(a, b) __umulhi(a, b)
uint32_t mul_hi (uint32_t a, uint32_t b)
{
	uint64_t result = (uint64_t) a * (uint64_t) b;
	return  (uint32_t) (result>>32);
}

//#define clz(a) __clz(a)
uint32_t clz (uint32_t a)
{
	uint32_t result = 0;
	for(int i=31;i>=0;i--){
		if(((a>>i)&1) == 1)
			result ++;
		else
			break;
	}
	return result;
}
//#define popcount(a) __popc(a)
uint32_t popcount (uint32_t a)
{
	uint32_t result = 0;
	for(int i=31;i>=0;i--){
		if(((a>>i)&1) == 1)
			result ++;
	}
	return result;
}

void swap(int *a, int *b)
{
	int t = *a;
	*a = *b;
	*b = t;
}


uint32_t fnv1a(uint32_t *h, uint32_t d)
{
        return *h = (*h ^ d) * 0x1000193;
}

// Implementation based on:
// https://github.com/mjosaarinen/tiny_sha3/blob/master/sha3.c
// converted from 64->32 bit words
const uint32_t keccakf_rndc[24] = {
	0x00000001, 0x00008082, 0x0000808a, 0x80008000, 0x0000808b, 0x80000001,
	0x80008081, 0x00008009, 0x0000008a, 0x00000088, 0x80008009, 0x8000000a,
	0x8000808b, 0x0000008b, 0x00008089, 0x00008003, 0x00008002, 0x00000080,
	0x0000800a, 0x8000000a, 0x80008081, 0x00008080, 0x80000001, 0x80008008
};

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

uint64_t keccak_f800(hash32_t header, uint64_t seed, uint32_t *result)
{
	uint32_t st[25];

	for (int i = 0; i < 25; i++)
		st[i] = 0;
	for (int i = 0; i < 8; i++)
		st[i] = header.uint32s[i];
	st[8] = seed;
	st[9] = seed >> 32;
	st[10] = result[0];
	st[11] = result[1];
	st[12] = result[2];
	st[13] = result[3];

	for (int r = 0; r < 21; r++) {
		keccak_f800_round(st, r);
	}
	// last round can be simplified due to partial output
	keccak_f800_round(st, 21);

	return (uint64_t)st[1] << 32 | st[0];
}

typedef struct {
	uint32_t z, w, jsr, jcong;
} kiss99_t;

// KISS99 is simple, fast, and passes the TestU01 suite
// https://en.wikipedia.org/wiki/KISS_(algorithm)
// http://www.cse.yorku.ca/~oz/marsaglia-rng.html
uint32_t kiss99(kiss99_t * st)
{
	uint32_t znew = (st->z = 36969 * (st->z & 65535) + (st->z >> 16));
	uint32_t wnew = (st->w = 18000 * (st->w & 65535) + (st->w >> 16));
	uint32_t MWC = ((znew << 16) + wnew);
	uint32_t SHR3 = (st->jsr ^= (st->jsr << 17), st->jsr ^= (st->jsr >> 13), st->jsr ^= (st->jsr << 5));
	uint32_t CONG = (st->jcong = 69069 * st->jcong + 1234567);
	return ((MWC^CONG) + SHR3);
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
	st.z = fnv1a(&fnv_hash, seed);
	st.w = fnv1a(&fnv_hash, seed >> 32);
	st.jsr = fnv1a(&fnv_hash, lane_id);
	st.jcong = fnv1a(&fnv_hash, lane_id);
	for (int i = 0; i < PROGPOW_REGS; i++)
		mix[i] = kiss99(&st);
}

kiss99_t progPowInit(uint64_t prog_seed, int mix_seq[PROGPOW_REGS])
{
	kiss99_t prog_rnd;
	uint32_t fnv_hash = 0x811c9dc5;
	prog_rnd.z = fnv1a(&fnv_hash, prog_seed);
	prog_rnd.w = fnv1a(&fnv_hash, prog_seed >> 32);
	prog_rnd.jsr = fnv1a(&fnv_hash, prog_seed);
	prog_rnd.jcong = fnv1a(&fnv_hash, prog_seed >> 32);
	// Create a random sequence of mix destinations for merge()
	// guaranteeing every location is touched once
	// Uses Fisher–Yates shuffle
	for (int i = 0; i < PROGPOW_REGS; i++)
		mix_seq[i] = i;
	for (int i = PROGPOW_REGS - 1; i > 0; i--)
	{
		int j = kiss99(&prog_rnd) % (i + 1);
		swap(&(mix_seq[i]), &(mix_seq[j]));
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
		case 2: *a = ROTL32(*a, ((r >> 16) % 32)) ^ b; break;
		case 3: *a = ROTR32(*a, ((r >> 16) % 32)) ^ b; break;
	}
}

// Random math between two input values
uint32_t math(uint32_t a, uint32_t b, uint32_t r)
{
	switch (r % 11)
	{
		case 0: return a + b; break;
		case 1: return a * b; break;
		case 2: return mul_hi(a, b); break;
		case 3: return min(a, b); break;
		case 4: return ROTL32(a, b); break;
		case 5: return ROTR32(a, b); break;
		case 6: return a & b; break;
		case 7: return a | b; break;
		case 8: return a ^ b; break;
		case 9: return clz(a) + clz(b); break;
		case 10: return popcount(a) + popcount(b); break;
		default: return 0;
	}
	return 0;
}

// Helper to get the next value in the per-program random sequence
#define rnd()    (kiss99(&prog_rnd))
// Helper to pick a random mix location
#define mix_src() (rnd() % PROGPOW_REGS)
// Helper to access the sequence of mix destinations
#define mix_dst() (mix_seq[(mix_seq_cnt++)%PROGPOW_REGS])

void progPowLoop(
	const uint64_t prog_seed,
	const uint32_t loop,
	uint32_t mix[PROGPOW_LANES][PROGPOW_REGS],
	const uint64_t *g_dag,
	const uint32_t *c_dag,
	const uint32_t progpow_dag_words)
{
	// All lanes share a base address for the global load
	// Global offset uses mix[0] to guarantee it depends on the load result
	uint32_t offset_g = mix[loop%PROGPOW_LANES][0] % progpow_dag_words;
	// Lanes can execute in parallel and will be convergent
	for (int l = 0; l < PROGPOW_LANES; l++)
	{
		// global load to sequential locations
		uint64_t data64 = g_dag[offset_g + l];

		// initialize the seed and mix destination sequence
		int mix_seq[PROGPOW_REGS];
		int mix_seq_cnt = 0;
		kiss99_t prog_rnd = progPowInit(prog_seed, mix_seq);

		uint32_t offset, data32;
		//int max_i = max(PROGPOW_CNT_CACHE, PROGPOW_CNT_MATH);
		int max_i;
		if (PROGPOW_CNT_CACHE > PROGPOW_CNT_MATH)
			max_i = PROGPOW_CNT_CACHE;
		else
			max_i = PROGPOW_CNT_MATH;
		for (int i = 0; i < max_i; i++)
		{
			if (i < PROGPOW_CNT_CACHE)
			{
				// Cached memory access
				// lanes access random location
				offset = mix[l][mix_src()] % PROGPOW_CACHE_WORDS;
				data32 = c_dag[offset];
				merge(&(mix[l][mix_dst()]), data32, rnd());
			}
			if (i < PROGPOW_CNT_MATH)
			{
				// Random Math
				data32 = math(mix[l][mix_src()], mix[l][mix_src()], rnd());
				merge(&(mix[l][mix_dst()]), data32, rnd());
			}
		}
		// Consume the global load data at the very end of the loop
		// Allows full latency hiding
		merge(&(mix[l][0]), data64, rnd());
		merge(&(mix[l][mix_dst()]), data64>>32, rnd());
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

	const uint64_t *g_dag = (uint64_t *) full_nodes;

	const hash32_t header;
	memcpy((void *)&header, (void *)&header_hash, sizeof(header_hash));
	uint32_t c_dag[PROGPOW_CACHE_WORDS];
	for(int threadIdx = 0; threadIdx<PROGPOW_LANES; threadIdx++)
	{
		// Load random data into the cache
		// TODO: should be a new blob of data, not existing DAG data
		for (uint32_t word = threadIdx*2; word < PROGPOW_CACHE_WORDS; word += PROGPOW_LANES*2)
		{
			uint64_t data = g_dag[word];
			c_dag[word + 0] = data;
			c_dag[word + 1] = data >> 32;
		}
	}

	if(!full_nodes){
		printf("Error, the client does not support light node at the moment\n");
		//printf("random output, do not trust! light %d full_size %d\n", (uint32_t) light, (uint32_t)full_size);
		exit(1);
	}

	uint32_t mix[PROGPOW_LANES][PROGPOW_REGS];
	uint32_t result[4];
	for (int i = 0; i < 4; i++)
		result[i] = 0;

	// keccak(header..nonce)
	uint64_t seed = keccak_f800(header, nonce, result);

	// initialize mix for all lanes
	for (int l = 0; l < PROGPOW_LANES; l++)
		fill_mix(seed, l, mix[l]);

	uint32_t dagWords = (unsigned)(full_size / PROGPOW_MIX_BYTES);
	// execute the randomly generated inner loop
	for (int i = 0; i < PROGPOW_CNT_MEM; i++)
	{
		if(full_nodes)
			progPowLoop(block_number, i, mix, g_dag, c_dag, dagWords);
		else
			progPowLoop(light->block_number, i, mix, g_dag, c_dag, dagWords);
	}


	// Reduce mix data to a single per-lane result
	uint32_t lane_hash[PROGPOW_LANES];
	for (int l = 0; l < PROGPOW_LANES; l++)
	{
		lane_hash[l] = 0x811c9dc5;
		for (int i = 0; i < PROGPOW_REGS; i++)
			fnv1a(&lane_hash[l], mix[l][i]);
	}
	// Reduce all lanes to a single 128-bit result
	for (int i = 0; i < 4; i++)
		result[i] = 0x811c9dc5;
	for (int l = 0; l < PROGPOW_LANES; l++)
		fnv1a(&result[l%4], lane_hash[l]);


	memset((void *)&ret->mix_hash, 0, sizeof(ret->mix_hash));
	memcpy(&ret->mix_hash, result, sizeof(result));
	memset((void *)&ret->result, 0, sizeof(ret->result));
	keccak_f800(header, seed, result);
	memcpy((void *)&ret->result, (void *)&header, sizeof(ret->result));


	//	if (full_size % MIX_WORDS != 0) {
	//		return false;
	//	}
	//
	//	// pack hash and nonce together into first 40 bytes of s_mix
	//	assert(sizeof(node) * 8 == 512);
	//	node s_mix[MIX_NODES + 1];
	//	memcpy(s_mix[0].bytes, &header_hash, 32);
	//	fix_endian64(s_mix[0].double_words[4], nonce);
	//
	//	// compute sha3-512 hash and replicate across mix
	//	SHA3_512(s_mix->bytes, s_mix->bytes, 40);
	//	fix_endian_arr32(s_mix[0].words, 16);
	//
	//	node* const mix = s_mix + 1;
	//	for (uint32_t w = 0; w != MIX_WORDS; ++w) {
	//		mix->words[w] = s_mix[0].words[w % NODE_WORDS];
	//	}
	//
	//	unsigned const page_size = sizeof(uint32_t) * MIX_WORDS;
	//	unsigned const num_full_pages = (unsigned) (full_size / page_size);
	//
	//	for (unsigned i = 0; i != ETHASH_ACCESSES; ++i) {
	//		uint32_t const index = fnv_hash(s_mix->words[0] ^ i, mix->words[i % MIX_WORDS]) % num_full_pages;
	//
	//		for (unsigned n = 0; n != MIX_NODES; ++n) {
	//			node const* dag_node;
	//			node tmp_node;
	//			if (full_nodes) {
	//				dag_node = &full_nodes[MIX_NODES * index + n];
	//			} else {
	//				ethash_calculate_dag_item(&tmp_node, index * MIX_NODES + n, light);
	//				dag_node = &tmp_node;
	//			}
	//
	//			{
	//				for (unsigned w = 0; w != NODE_WORDS; ++w) {
	//					mix[n].words[w] = fnv_hash(mix[n].words[w], dag_node->words[w]);
	//				}
	//			}
	//		}
	//
	//	}
	//
	//	fix_endian_arr32(mix->words, MIX_WORDS / 4);
	//	memcpy(&ret->mix_hash, mix->bytes, 32);
	//	// final Keccak hash
	//	SHA3_256(&ret->result, s_mix->bytes, 64 + 32); // Keccak-256(s + compressed_mix)
	return true;
}

void ethash_quick_hash(
	ethash_h256_t* return_hash,
	ethash_h256_t const* header_hash,
	uint64_t nonce,
	ethash_h256_t const* mix_hash
)
{
	uint8_t buf[64 + 32];
	memcpy(buf, header_hash, 32);
	fix_endian64_same(nonce);
	memcpy(&(buf[32]), &nonce, 8);
	SHA3_512(buf, buf, 40);
	memcpy(&(buf[64]), mix_hash, 32);
	SHA3_256(return_hash, buf, 64 + 32);
}

ethash_h256_t ethash_get_seedhash(uint64_t block_number)
{
	ethash_h256_t ret;
	ethash_h256_reset(&ret);
	uint64_t const epochs = block_number / ETHASH_EPOCH_LENGTH;
	for (uint32_t i = 0; i < epochs; ++i)
		SHA3_256(&ret, (uint8_t*)&ret, 32);
	return ret;
}

bool ethash_quick_check_difficulty(
	ethash_h256_t const* header_hash,
	uint64_t const nonce,
	ethash_h256_t const* mix_hash,
	ethash_h256_t const* boundary
)
{

	ethash_h256_t return_hash;
	ethash_quick_hash(&return_hash, header_hash, nonce, mix_hash);
	return ethash_check_difficulty(&return_hash, boundary);
}

ethash_light_t ethash_light_new_internal(uint64_t cache_size, ethash_h256_t const* seed)
{
	struct ethash_light *ret;
	ret = calloc(sizeof(*ret), 1);
	if (!ret) {
		return NULL;
	}
#if defined(__MIC__)
	ret->cache = _mm_malloc((size_t)cache_size, 64);
#else
	ret->cache = malloc((size_t)cache_size);
#endif
	if (!ret->cache) {
		goto fail_free_light;
	}
	node* nodes = (node*)ret->cache;
	if (!ethash_compute_cache_nodes(nodes, cache_size, seed)) {
		goto fail_free_cache_mem;
	}
	ret->cache_size = cache_size;
	return ret;

fail_free_cache_mem:
#if defined(__MIC__)
	_mm_free(ret->cache);
#else
	free(ret->cache);
#endif
fail_free_light:
	free(ret);
	return NULL;
}

ethash_light_t ethash_light_new(uint64_t block_number)
{
	ethash_h256_t seedhash = ethash_get_seedhash(block_number);
	ethash_light_t ret;
	ret = ethash_light_new_internal(ethash_get_cachesize(block_number), &seedhash);
	ret->block_number = block_number;
	return ret;
}

void ethash_light_delete(ethash_light_t light)
{
	if (light->cache) {
		free(light->cache);
	}
	free(light);
}

ethash_return_value_t ethash_light_compute_internal(
	ethash_light_t light,
	uint64_t full_size,
	ethash_h256_t const header_hash,
	uint64_t nonce
)
{
	ethash_return_value_t ret;
	ret.success = true;
	if (!progpow_hash(&ret, NULL, light, full_size, header_hash, nonce, 0)) {
		ret.success = false;
	}
	return ret;
}

ethash_return_value_t ethash_light_compute(
	ethash_light_t light,
	ethash_h256_t const header_hash,
	uint64_t nonce
)
{
	uint64_t full_size = ethash_get_datasize(light->block_number);
	return ethash_light_compute_internal(light, full_size, header_hash, nonce);
}

static bool ethash_mmap(struct ethash_full* ret, FILE* f)
{
	int fd;
	char* mmapped_data;
	errno = 0;
	ret->file = f;
	if ((fd = ethash_fileno(ret->file)) == -1) {
		return false;
	}
	mmapped_data= mmap(
		NULL,
		(size_t)ret->file_size + ETHASH_DAG_MAGIC_NUM_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fd,
		0
	);
	if (mmapped_data == MAP_FAILED) {
		return false;
	}
	ret->data = (node*)(mmapped_data + ETHASH_DAG_MAGIC_NUM_SIZE);
	return true;
}

ethash_full_t ethash_full_new_internal(
	char const* dirname,
	ethash_h256_t const seed_hash,
	uint64_t full_size,
	ethash_light_t const light,
	ethash_callback_t callback
)
{
	struct ethash_full* ret;
	FILE *f = NULL;
	ret = calloc(sizeof(*ret), 1);
	if (!ret) {
		return NULL;
	}
	ret->file_size = (size_t)full_size;

	enum ethash_io_rc err = ethash_io_prepare(dirname, seed_hash, &f, (size_t)full_size, false);
	if (err == ETHASH_IO_FAIL)
		goto fail_free_full;

	if (err == ETHASH_IO_MEMO_SIZE_MISMATCH) {
		// if a DAG of same filename but unexpected size is found, silently force new file creation
		if (ethash_io_prepare(dirname, seed_hash, &f, (size_t)full_size, true) != ETHASH_IO_MEMO_MISMATCH) {
			ETHASH_CRITICAL("Could not recreate DAG file after finding existing DAG with unexpected size.");
			goto fail_free_full;
		}
		// we now need to go through the mismatch case, NOT the match case
		err = ETHASH_IO_MEMO_MISMATCH;
	}

	if (err == ETHASH_IO_MEMO_MISMATCH || err == ETHASH_IO_MEMO_MATCH) {
		if (!ethash_mmap(ret, f)) {
			ETHASH_CRITICAL("mmap failure()");
			goto fail_close_file;
		}

		if (err == ETHASH_IO_MEMO_MATCH) {
#if defined(__MIC__)
			node* tmp_nodes = _mm_malloc((size_t)full_size, 64);
			//copy all nodes from ret->data
			//mmapped_nodes are not aligned properly
			uint32_t const countnodes = (uint32_t) ((size_t)ret->file_size / sizeof(node));
			//fprintf(stderr,"ethash_full_new_internal:countnodes:%d",countnodes);
			for (uint32_t i = 1; i != countnodes; ++i) {
				tmp_nodes[i] = ret->data[i];
			}
			ret->data = tmp_nodes;
#endif
			return ret;
		}
	}


#if defined(__MIC__)
	ret->data = _mm_malloc((size_t)full_size, 64);
#endif
	if (!ethash_compute_full_data(ret->data, full_size, light, callback)) {
		ETHASH_CRITICAL("Failure at computing DAG data.");
		goto fail_free_full_data;
	}

	// after the DAG has been filled then we finalize it by writting the magic number at the beginning
	if (fseek(f, 0, SEEK_SET) != 0) {
		ETHASH_CRITICAL("Could not seek to DAG file start to write magic number.");
		goto fail_free_full_data;
	}
	uint64_t const magic_num = ETHASH_DAG_MAGIC_NUM;
	if (fwrite(&magic_num, ETHASH_DAG_MAGIC_NUM_SIZE, 1, f) != 1) {
		ETHASH_CRITICAL("Could not write magic number to DAG's beginning.");
		goto fail_free_full_data;
	}
	if (fflush(f) != 0) {// make sure the magic number IS there
		ETHASH_CRITICAL("Could not flush memory mapped data to DAG file. Insufficient space?");
		goto fail_free_full_data;
	}
	return ret;

fail_free_full_data:
	// could check that munmap(..) == 0 but even if it did not can't really do anything here
	munmap(ret->data, (size_t)full_size);
#if defined(__MIC__)
	_mm_free(ret->data);
#endif
fail_close_file:
	fclose(ret->file);
fail_free_full:
	free(ret);
	return NULL;
}

ethash_full_t ethash_full_new(ethash_light_t light, ethash_callback_t callback)
{
	char strbuf[256];
	if (!ethash_get_default_dirname(strbuf, 256)) {
		return NULL;
	}
	uint64_t full_size = ethash_get_datasize(light->block_number);
	ethash_h256_t seedhash = ethash_get_seedhash(light->block_number);
	return ethash_full_new_internal(strbuf, seedhash, full_size, light, callback);
}

void ethash_full_delete(ethash_full_t full)
{
	// could check that munmap(..) == 0 but even if it did not can't really do anything here
	munmap(full->data, (size_t)full->file_size);
	if (full->file) {
		fclose(full->file);
	}
	free(full);
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

void const* ethash_full_dag(ethash_full_t full)
{
	return full->data;
}

uint64_t ethash_full_dag_size(ethash_full_t full)
{
	return full->file_size;
}
