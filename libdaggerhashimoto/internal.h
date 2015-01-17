// -*-c-*-
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
/** @file daggerhashimoto.h
 * @author Matthew Wampler-Doty <matt@w-d.org>
 * @date 2014
 */

#pragma once
#include "compiler.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

#define SAFE_PRIME 4294967087U
#define SAFE_PRIME_TOTIENT 4294967086U
#define SAFE_PRIME_TOTIENT_TOTIENT 2147483542
#define SAFE_PRIME2 4294965887U
#define NUM_BITS 64U
#define WIDTH 64U      // Must be a multiple of 4
#define HASH_CHARS 32
#define HASH_UINT64S 4

typedef struct {
    uint64_t dag_size;              // Size of the dataset, in blocks of data determined by WIDTH
    unsigned char diff[HASH_CHARS]; // Difficulty (adjusted during block evaluation)
    int epoch_time;                 // Length of an epoch in blocks (how often the dataset is updated)
    int n_inc;                      // Increment in value of n per period epoch
    uint64_t cache_size;		    // How big should the light client's cache be? (Must be a least WIDTH)
    int accesses;                   // Number of dataset accesses during hashimoto
  } parameters;

/* C99 initialisers not supported by Visual Studio or C++ */
const parameters defaults = {
        /*.dag_size    = */ ((uint64_t)4000055296 * 8) / NUM_BITS,
        /*.diff		   = */ {
                              126, 126, 126, 126, 126, 126, 126, 126,
                              126, 126, 126, 126, 126, 126, 126, 126,
                              126, 126, 126, 126, 126, 126, 126, 126,
                              126, 126, 126, 126, 126, 126, 126, 126,},
		/*.epoch_time  = */ 1000,
		/*.n_inc       = */ 65536,
        /*.cache_size  = */ 4,  // CANNOT BE LESS THAN 4!
		/*.accesses    = */ 200,
};

void sha3_dag(uint8_t result[HASH_CHARS], const uint8_t previous_hash[HASH_CHARS]);
void sha3_rand(uint64_t out[HASH_UINT64S], const uint8_t previous_hash[HASH_CHARS], const uint64_t nonce);

#ifdef __cplusplus
}
#endif // __cplusplus