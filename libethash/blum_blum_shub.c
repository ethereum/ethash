#include <stdint.h>
#include "blum_blum_shub.h"

void init_power_table_mod_prime1(uint32_t table[32], const uint32_t n) {
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
    uint32_t q = three_pow_mod_totient1(
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