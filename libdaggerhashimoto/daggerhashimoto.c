#include <stdlib.h>
#include "daggerhashimoto.h"
#include "sha3.h"

void sha3_1(uint8_t result[HASH_CHARS], const unsigned char previous_hash[HASH_CHARS]) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    sha3_update(&ctx, previous_hash, HASH_CHARS);
    sha3_finalize(&ctx, result);
}

// TODO: Switch to little endian
void sha3_dag(uint64_t *dag, const unsigned char previous_hash[HASH_CHARS]) {
    // DAG must be at least 256 bits long!
    uint8_t result[HASH_CHARS];
    int i, j;
    sha3_1(result, previous_hash);
    for (i = 0; i < HASH_UINT64S; ++i) {
        dag[i] = 0;
        for (j = 0; j < 8; ++j) {
            dag[i] <<= 8;
            dag[i] += result[8 * i + j];
        }
    }
}

void uint64str(uint8_t result[8], uint64_t n) {
    int i;
    for (i = 0; i < 8; ++i) {
        result[i] = (uint8_t) n;
        n >>= 8;
    }
}

// TODO: Switch to little endian
void sha3_rand(
        uint64_t out[HASH_UINT64S],
        const unsigned char previous_hash[HASH_CHARS],
        const uint64_t nonce) {
    uint8_t result[HASH_CHARS], nonce_data[8];
    int i, j;
    struct sha3_ctx ctx;
    uint64str(nonce_data, nonce);
    sha3_init(&ctx, 256);
    sha3_update(&ctx, previous_hash, HASH_CHARS);
    sha3_update(&ctx, nonce_data, 8);
    sha3_finalize(&ctx, result);
    for (i = 0; i < HASH_UINT64S; ++i) {
        out[i] = 0;
        for (j = 0; j < 8; ++j) {
            out[i] <<= 8;
            out[i] += result[8 * i + j];
        }
    }
}

// TODO: Test Me
void sha3_mix(
        uint8_t result[HASH_CHARS],
        const uint64_t mix[WIDTH],
        const uint64_t nonce) {
    uint8_t temp[8];
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    for(int i = 0; i < WIDTH; ++i) {
        uint64str(temp, mix[i]);
        sha3_update(&ctx, temp, 8);
    }
    uint64str(temp, nonce);
    sha3_update(&ctx, temp, 8);
    sha3_finalize(&ctx, result);
}

inline uint32_t cube_mod_safe_prime(const uint32_t x) {
    uint64_t temp = x;
    temp *= x;
    temp %= SAFE_PRIME;
    temp *= x;
    return (uint32_t) (temp % SAFE_PRIME);
}

inline uint32_t cube_mod_safe_prime2(const uint32_t x) {
    uint64_t temp = x;
    temp *= x;
    temp %= SAFE_PRIME2;
    temp *= x;
    return (uint32_t) (temp % SAFE_PRIME2);
}

// TODO: Test Me
void produce_dag(
        uint64_t *dag,
        const parameters params,
        const unsigned char seed[HASH_CHARS]) {
    sha3_dag(dag, seed);
    uint32_t picker1 = (uint32_t) dag[0] % SAFE_PRIME,
            picker2, worker1, worker2;
    uint64_t x;
    size_t i;
    int j;
    picker1 = picker1 < 2 ? 2 : picker1;
    for (i = 8; i < params.n; ++i) {
        picker2 = cube_mod_safe_prime(picker1);
        worker1 = picker1 = cube_mod_safe_prime(picker2);
        x = ((uint64_t) picker2 << 32) + picker1;
        x ^= dag[x % i];
        for (j = 0; j < params.w; ++j) {
            worker2 = cube_mod_safe_prime2(worker1);
            worker1 = cube_mod_safe_prime2(worker2);
            x ^= ((uint64_t) worker2 << 32) + worker1;
        }
        dag[i] = x;
    }
}

const uint32_t powers_of_three_mod_totient[32] = {
        3, 9, 81, 6561, 43046721, 3884235087U, 4077029855U, 106110483U,
        2292893763U, 2673619771U, 2265535291U, 2641962139U, 867632699U,
        4234161123U, 4065670495U, 1161610561U, 1960994291U, 683176121U,
        1539788995U, 1214448689U, 2554812497U, 2574646649U, 3290602031U,
        2381552417U, 3391635653U, 639421717U, 1685119297U, 4206074945U,
        1006214457U, 102532655U, 4081098815U, 3106101787U
};

uint32_t three_pow_mod_totient(uint32_t p) {
    uint64_t r = 1;
    for(int i = 0; i < 32; ++i) {
        if (p & 1) {
            r *= powers_of_three_mod_totient[i];
            r %= (SAFE_PRIME - 1);
        }
        p >>= 1;
    }
    return (uint32_t) r;
}

void init_power_table_mod_prime(uint32_t table[32], const uint32_t n) {
    uint64_t r = n;
    r %= SAFE_PRIME;
    table[0] = (uint32_t) r;
    for(int i = 1; i < 32; ++i) {
        r *= r;
        r %= SAFE_PRIME;
        table[i] = (uint32_t) r;
    }
}

uint32_t quick_bbs(const uint32_t table[32], const uint32_t p) {
    uint32_t q = three_pow_mod_totient(p);
    uint64_t r = 1;
    for(int i = 0; i < 32; ++i) {
        if (q & 1) {
            r *= table[i];
            r %= SAFE_PRIME;
        }
        q >>= 1;
    }
    return (uint32_t) r;
}

uint64_t quick_calc_cached(uint64_t *cache, const parameters params, uint64_t pos) {
//    if (pos < params.cache_size)
//        return cache[pos]; // todo, 64->32 bit truncation
//    else {
//        uint32_t x = pow_mod(cache[0], pos + 1);  // todo, 64->32 bit truncation
//        int j;
//        for (j = 0; j < params.w; ++j)
//            x ^= cube_mod_safe_prime(x);
//        return x;
//    }
}

uint32_t quick_calc(
        parameters params,
        const unsigned char seed[HASH_CHARS],
        const uint64_t pos) {
    uint64_t *cache = alloca(sizeof(uint64_t) * params.cache_size); // might be too large for stack?
    params.n = params.cache_size;
    produce_dag(cache, params, seed);
    return quick_calc_cached(cache, params, pos);
}

void hashimoto(
        unsigned char result[HASH_CHARS],
        const uint64_t *dag,
        const parameters params,
        const unsigned char previous_hash[HASH_CHARS],
        const uint64_t nonce) {
    uint64_t rand[HASH_UINT64S];
    const uint64_t m = params.n - WIDTH;
    sha3_rand(rand, previous_hash, nonce);
    uint64_t mix[WIDTH];
    int i, j, p;
    for (i = 0; i < WIDTH; ++i) {
        mix[i] = dag[rand[0] % m + i];
    }
    for (p = 0; p < params.accesses; ++p) {
        uint64_t ind = mix[p % WIDTH] % m;
        for (i = 0; i < WIDTH; ++i)
            mix[i] ^= dag[ind + i];
    }
    sha3_mix(result, mix, nonce);
}


void quick_hashimoto_cached(
        unsigned char result[32],
        uint64_t *cache,
        const parameters params,
        const unsigned char previous_hash[32],
        const uint64_t nonce) {
    uint64_t rand[HASH_UINT64S];
    const uint64_t m = params.n - WIDTH;
    sha3_rand(rand, previous_hash, nonce);
    uint64_t mix[WIDTH];
    int i, p;
    for (i = 0; i < WIDTH; ++i)
        mix[i] = 0;
    for (p = 0; p < params.accesses; ++p) {
        uint64_t ind = mix[p % WIDTH] % params.n;
        for (i = 0; i < WIDTH; ++i)
            mix[i] ^= quick_calc_cached(cache, params, ind + i);
    }
    sha3_mix(result, mix, nonce);
}

void quick_hashimoto(
        unsigned char result[32],
        const unsigned char seed[32],
        parameters params,
        const unsigned char previous_hash[32],
        const uint64_t nonce) {
    const uint64_t original_n = params.n;
    uint64_t *cache = alloca(sizeof(uint64_t) * params.cache_size); // might be too large for stack?
    params.n = params.cache_size;
    produce_dag(cache, params, previous_hash);
    params.n = original_n;
    quick_hashimoto_cached(result, cache, params, previous_hash, nonce);
}