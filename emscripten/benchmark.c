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

/** @file benchmark.c
* @author Matthew Wampler-Doty <negacthulhu@gmail.com>
* @date 2015
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libethash/ethash.h>

#define TRIALS 100
uint8_t g_hash[32];

int main(void) {
    ethash_params params;
    ethash_params_init(&params);
    params.full_size = 262147 * 4096;	// 1GBish;
    params.cache_size = 8209*4096;
    void* mem_buf = malloc(params.cache_size);
    uint8_t seed[32], previous_hash[32];
    memcpy(seed, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    memcpy(previous_hash, "~~X~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);

    ethash_cache cache;
    ethash_cache_init(&cache, mem_buf);

    printf("Building cache...\n");
    clock_t startTime = clock();
    ethash_mkcache(&cache, &params, seed);
    clock_t endTime = clock();
    printf("Time to build Cache: %lu msecs\n", (unsigned long) ((endTime - startTime) * 1000.0 / CLOCKS_PER_SEC));

    printf("Verifying...\n");
    for (unsigned read_size = 4096; read_size <= 4096*256; read_size <<= 1)
    {
        params.hash_read_size = read_size;

        startTime = clock();


        for (int nonce = 0; nonce < TRIALS; ++nonce)
        {
            ethash_light(g_hash, &cache, &params, previous_hash, nonce);
        }
        clock_t time = clock() - startTime;

        printf("read_size %5ukb, hashrate: %6g per second\n",
                read_size / 1024,
                ((TRIALS * CLOCKS_PER_SEC * 1.0)/time)
        );
    }

    free(mem_buf);
    return EXIT_SUCCESS;
}