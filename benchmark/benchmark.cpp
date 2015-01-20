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
/** @file benchmark.cpp
 * @author Tim Hughes <tim@ethdev.org>
 * @date 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "libethash/ethash.h"
#include "libethash/util.h"

uint8_t g_hash[32];

extern "C" int main(void)
{
	// params for ethash
	ethash_params params;
	ethash_params_init(&params);
	params.full_size = 262147 * 4096;	// 1GBish;
	params.cache_size = 8209*4096;
	params.k = 2 * (params.full_size / params.cache_size);
	uint8_t seed[32], previous_hash[32];
	memcpy(seed, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
	memcpy(previous_hash, "~~X~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);

	const unsigned trials = 1000;
	
	// make random seed and header
	uint8_t header[32];
	srand(3248723843U);
	for (unsigned i = 0; i != 32; ++i)
	{
		header[i] = (uint8_t) rand();
	}

	// allocate page aligned buffer for dataset
	void* mem_buf = malloc(params.full_size + 4095 + 64);
	void* mem = (void*)((uintptr_t(mem_buf) + 4095) & ~4095);

	ethash_cache cache;
	ethash_cache_init(&cache, mem);

	// compute cache or full data
	{
		clock_t startTime = clock();

		#ifdef FULL
			ethash_compute_full_data(cache.mem, &params, seed);
		#else
			ethash_mkcache(&cache, &params, seed);
		#endif // FULL

		clock_t time = clock() - startTime;

		#ifdef FULL
			debugf("ethash_compute_full_data: %ums\n", (unsigned)((time*1000)/CLOCKS_PER_SEC));
		#else
			debugf("ethash_mkcache: %ums\n", (unsigned)((time*1000)/CLOCKS_PER_SEC));
		#endif // FULL
	}

	// trial different numbers of accesses
	for (unsigned read_size = 4096; read_size <= 4096*256; read_size <<= 1)
	{
		params.hash_read_size = read_size;
		
		clock_t startTime = clock();

		// uncomment to enable multicore
		//#pragma omp parallel for
		for (int nonce = 0; nonce < trials; ++nonce)
		{
			#ifdef FULL
				ethash_full(g_hash, cache.mem, &params, previous_hash, nonce);
			#else
				ethash_light(g_hash, &cache, &params, previous_hash, nonce);
			#endif // FULL

			//if ((nonce % 500) == 0)
			//	logf("trials: %u\n", nonce);
		}
		clock_t time = clock() - startTime;

		debugf("read_size %5ukb, hashrate: %6u, bw: %5u MB/s\n",
			read_size / 1024,
			(unsigned)((trials*CLOCKS_PER_SEC)/time),
			(unsigned)((((uint64_t)trials*read_size*CLOCKS_PER_SEC)/time) / (1024*1024))
			);
	}

	free(mem_buf);

	return 0;
}

