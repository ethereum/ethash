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
#define WIDTH 64U
#define HASH_CHARS 32
#define HASH_UINT64S 4

typedef struct {
    size_t dag_size;                // Size of the dataset
    uint8_t diff[HASH_CHARS];       // Difficulty (adjusted during block evaluation)
    int epoch_time;                 // Length of an epoch in blocks (how often the dataset is updated)
    int n_inc;                      // Increment in value of n per period epoch
    size_t cache_size;				// How big should the light client's cache be?
    int work_factor;                // Work factor for memory free mining
    int accesses;                   // Number of dataset accesses during hashimoto
  } parameters;

/* C99 initialisers not supported by Visual Studio or C++ */
const parameters defaults = {
        /*.n           = */ ((uint64_t)4000055296 * 8) / NUM_BITS,
        /*.diff		   = */ {
                              126, 126, 126, 126, 126, 126, 126, 126,
                              126, 126, 126, 126, 126, 126, 126, 126,
                              126, 126, 126, 126, 126, 126, 126, 126,
                              126, 126, 126, 126, 126, 126, 126, 126,},
		/*.epoch_time  = */ 1000,
		/*.n_inc       = */ 65536,
        /*.cache_size  = */ 4,  // CANNOT BE LESS THAN 4!
		/*.work_factor = */ 3,
		/*.accesses    = */ 200,
};

void sha3_dag(uint8_t result[HASH_CHARS], const uint8_t previous_hash[HASH_CHARS]);
void sha3_rand(uint64_t out[HASH_UINT64S], const uint8_t previous_hash[HASH_CHARS], const uint64_t nonce);
uint32_t cube_mod_safe_prime(const uint32_t x);
uint32_t cube_mod_safe_prime2(const uint32_t x);
uint32_t three_pow_mod_totient(uint32_t p);
void init_power_table_mod_prime(uint32_t table[32], const uint32_t n);
uint32_t quick_bbs(const uint32_t power_table[32], const uint64_t p);

#ifdef __cplusplus
}
#endif // __cplusplus
