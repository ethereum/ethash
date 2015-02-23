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
#include <libethash/ethash.h>
#include <libethash/util.h>
#ifdef OPENCL
#include <libethash-cl/ethash_cl_miner.h>
#endif
#include <vector>

#ifdef WITH_CRYPTOPP
#include <libethash/SHA3_cryptopp.h>
#include <string>

#else
#include "libethash/sha3.h"
#endif // WITH_CRYPTOPP

#if defined(OPENCL)
const unsigned trials = 1024*1024;
#elif defined(FULL)
const unsigned trials = 1024*1024/8;
#else
const unsigned trials = 1024*1024/1024;
#endif
uint8_t g_hashes[32*trials];

static char nibbleToChar(unsigned nibble)
{
	return (char) ((nibble >= 10 ? 'a'-10 : '0') + nibble);
}

static uint8_t charToNibble(char chr)
{
	if (chr >= '0' && chr <= '9')
	{
		return (uint8_t) (chr - '0');
	}
	if (chr >= 'a' && chr <= 'z')
	{
		return (uint8_t) (chr - 'a' + 10);
	}
	if (chr >= 'A' && chr <= 'Z')
	{
		return (uint8_t) (chr - 'A' + 10);
	}
	return 0;
}

static std::vector<uint8_t> hexStringToBytes(char const* str)
{
	std::vector<uint8_t> bytes(strlen(str) >> 1);
	for (unsigned i = 0; i != bytes.size(); ++i)
	{
		bytes[i] = charToNibble(str[i*2 | 0]) << 4;
		bytes[i] |= charToNibble(str[i*2 | 1]);
	}
	return bytes;
}

static std::string bytesToHexString(uint8_t const* bytes, unsigned size)
{
	std::string str;
	for (unsigned i = 0; i != size; ++i)
	{
		str += nibbleToChar(bytes[i] >> 4);
		str += nibbleToChar(bytes[i] & 0xf);
	}
	return str;
}

extern "C" int main(void)
{
	// params for ethash
	ethash_params params;
	ethash_params_init(&params, 0);
	//params.full_size = 262147 * 4096;	// 1GBish;
	//params.full_size = 32771 * 4096;	// 128MBish;
	//params.full_size = 8209 * 4096;	// 8MBish;
	//params.cache_size = 8209*4096;
	//params.cache_size = 2053*4096;
	uint8_t seed[32], previous_hash[32];

	memcpy(seed, hexStringToBytes("9410b944535a83d9adf6bbdcc80e051f30676173c16ca0d32d6f1263fc246466").data(), 32);
	memcpy(previous_hash, hexStringToBytes("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470").data(), 32);
	
	// allocate page aligned buffer for dataset
#ifdef FULL
	void* full_mem_buf = malloc(params.full_size + 4095);
	void* full_mem = (void*)((uintptr_t(full_mem_buf) + 4095) & ~4095);
#endif
	void* cache_mem_buf = malloc(params.cache_size + 63);
	void* cache_mem = (void*)((uintptr_t(cache_mem_buf) + 63) & ~63);

	ethash_cache cache;
	ethash_cache_init(&cache, cache_mem);

	// compute cache or full data
	{
		clock_t startTime = clock();
		ethash_mkcache(&cache, &params, seed);
		clock_t time = clock() - startTime;

		uint8_t cache_hash[32];
		SHA3_256(cache_hash, (uint8_t const*)cache_mem, params.cache_size);
		debugf("ethash_mkcache: %ums, sha3: %s\n", (unsigned)((time*1000)/CLOCKS_PER_SEC), bytesToHexString(cache_hash,sizeof(cache_hash)).data());

		// print a couple of test hashes
		{
			const clock_t startTime = clock();
			ethash_light(g_hashes, &cache, &params, previous_hash, 0);
			const clock_t time = clock() - startTime;
			debugf("ethash_light test: %ums, %s\n", (unsigned)((time*1000)/CLOCKS_PER_SEC), bytesToHexString(g_hashes, 32).data());
		}

		#ifdef FULL
			startTime = clock();
			ethash_compute_full_data(full_mem, &params, &cache);
			time = clock() - startTime;
			debugf("ethash_compute_full_data: %ums\n", (unsigned)((time*1000)/CLOCKS_PER_SEC));
		#endif // FULL
	}

#ifdef OPENCL
	ethash_cl_miner miner;
	{
		const clock_t startTime = clock();
		if (!miner.init(params, seed))
			exit(-1);
		const clock_t time = clock() - startTime;
        debugf("ethash_cl_miner init: %ums\n", (unsigned)((time*1000)/CLOCKS_PER_SEC));
	}
#endif


#ifdef FULL
    {
        const clock_t startTime = clock();
        ethash_full(g_hashes, full_mem, &params, previous_hash, 0);
        const clock_t time = clock() - startTime;
        debugf("ethash_full test: %uns, %s\n", (unsigned)((time*1000000)/CLOCKS_PER_SEC), bytesToHexString(g_hashes, 32).data());
    }
#endif

#ifdef OPENCL
	// validate 1024 hashes against CPU
	miner.hashes(g_hashes, previous_hash, 0, 1024);
	for (unsigned i = 0; i != 1024; ++i)
	{
		uint8_t g_hash2[32];
		ethash_light(g_hash2, &cache, &params, previous_hash, i);
		if (memcmp(g_hash2, g_hashes + 32*i, 32) != 0)
		{
			debugf("nonce %u failed: %s %s\n", i, bytesToHexString(g_hashes + 32*i, 32).c_str(), bytesToHexString(g_hash2, 32).c_str());
			static unsigned c = 0;
			if (++c == 16)
			{
				exit(-1);
			}
		}
	}
#endif

	// trial different numbers of accesses
#ifdef OPENCL
	for (unsigned pass = 0; pass != 2; ++pass)
#endif
	for (unsigned read_size = 4096; read_size <= 4096*32; read_size <<= 1)
	{
		params.hash_read_size = read_size;

		clock_t startTime = clock();

		clock_t time = 0;
		#ifdef OPENCL
		{
			if (pass == 1)
			{
				time = miner.hashes(g_hashes, previous_hash, 0, trials, read_size, true);
			}
			else
			{
				miner.hashes(g_hashes, previous_hash, 0, trials, read_size, false);
			}
		}
		#else
		{
			//#pragma omp parallel for
			for (int nonce = 0; nonce < trials; ++nonce)
			{
				#ifdef FULL
					ethash_full(g_hashes + 32*nonce, full_mem, &params, previous_hash, nonce);
				#else
					ethash_light(g_hashes + 32*nonce, &cache, &params, previous_hash, nonce);
				#endif // FULL
			}
		}
		#endif
		if (!time)
		{
			time = clock() - startTime;
		}
		
		debugf(			
			"%sread_size %5ukb, hashrate: %8u, bw: %6u MB/s\n",
#ifdef OPENCL
			pass ? "inner " : "outer ",
#else
			"",
#endif
			read_size / 1024,
			(unsigned)(((uint64_t)trials*CLOCKS_PER_SEC)/time),
			(unsigned)((((uint64_t)trials*read_size*CLOCKS_PER_SEC)/time) / (1024*1024))
			);
	}

	free(cache_mem_buf);
#ifdef FULL
	free(full_mem_buf);
#endif

	return 0;
}

