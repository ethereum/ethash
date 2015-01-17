#include <stdint.h>
#include "blum_blum_shub.h"

uint32_t cube_mod_safe_prime1(const uint32_t x) {
    uint64_t temp = x;
    temp *= x;
    temp %= SAFE_PRIME;
    temp *= x;
    return (uint32_t) (temp % SAFE_PRIME);
}

uint32_t cube_mod_safe_prime2(const uint32_t x) {
    uint64_t temp = x;
    temp *= x;
    temp %= SAFE_PRIME2;
    temp *= x;
    return (uint32_t) (temp % SAFE_PRIME2);
}


static const uint32_t powers_of_three_mod_totient1[32] = {
        3, 9, 81, 6561, 43046721, 3884235087U, 4077029855U, 106110483U,
        2292893763U, 2673619771U, 2265535291U, 2641962139U, 867632699U,
        4234161123U, 4065670495U, 1161610561U, 1960994291U, 683176121U,
        1539788995U, 1214448689U, 2554812497U, 2574646649U, 3290602031U,
        2381552417U, 3391635653U, 639421717U, 1685119297U, 4206074945U,
        1006214457U, 102532655U, 4081098815U, 3106101787U
};

uint32_t three_pow_mod_totient1(uint32_t p) {
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