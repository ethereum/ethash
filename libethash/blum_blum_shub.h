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

/** @file blum_blum_shub.h
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

static inline uint32_t cube_mod_safe_prime1(const uint32_t x) {
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

static inline uint32_t three_pow_mod_totient1(uint32_t p) {
    static const uint32_t powers_of_three_mod_totient1[32] = {
            3, 9, 81, 6561, 43046721, 3884235087U, 4077029855U, 106110483U,
            2292893763U, 2673619771U, 2265535291U, 2641962139U, 867632699U,
            4234161123U, 4065670495U, 1161610561U, 1960994291U, 683176121U,
            1539788995U, 1214448689U, 2554812497U, 2574646649U, 3290602031U,
            2381552417U, 3391635653U, 639421717U, 1685119297U, 4206074945U,
            1006214457U, 102532655U, 4081098815U, 3106101787U
    };
    uint64_t r = 1;
    for(int i = 0; i < 32; ++i) {
        if (p & 1) {
            r *= powers_of_three_mod_totient1[i];
            r %= SAFE_PRIME_TOTIENT;
        }
        p >>= 1;
    }
    return (uint32_t) r;
}

void init_power_table_mod_prime1(uint32_t table[32], const uint32_t n);
uint32_t quick_bbs(const uint32_t power_table[32], const uint64_t p);

static inline uint32_t make_seed1(uint64_t seed)
{
	return clamp_u32((uint32_t)(seed % SAFE_PRIME), 2, SAFE_PRIME-2);
}

static inline uint32_t make_seed2(uint64_t seed)
{
	return clamp_u32((uint32_t)(seed % SAFE_PRIME2), 2, SAFE_PRIME2-2);
}

#ifdef __cplusplus
}
#endif // __cplusplus
