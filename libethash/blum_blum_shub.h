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
/** @file ethash.h
 * @author Matthew Wampler-Doty <tim@ethdev.org>
 * @date 2015
 */
#pragma once
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SAFE_PRIME 4294967087U
#define SAFE_PRIME_TOTIENT 4294967086U
#define SAFE_PRIME_TOTIENT_TOTIENT 2147483542
#define SAFE_PRIME2 4294965887U

// maybe want to inline some of these?

uint32_t cube_mod_safe_prime1(const uint32_t x);
uint32_t cube_mod_safe_prime2(const uint32_t x);
uint32_t three_pow_mod_totient1(uint32_t p);
void init_power_table_mod_prime1(uint32_t table[32], const uint32_t n);
uint32_t quick_bbs(const uint32_t power_table[32], const uint64_t p);

static inline uint32_t make_seed(uint64_t seed, uint32_t safe_prime)
{
	return clamp_u32((uint32_t)(seed % safe_prime), 2, safe_prime-2);
}

#ifdef __cplusplus
}
#endif // __cplusplus
