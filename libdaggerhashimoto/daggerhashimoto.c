#include <stdlib.h>
#include <string.h>
#include "daggerhashimoto.h"
#include "sha3.h"

void sha3_dag(uint8_t result[HASH_CHARS], const unsigned char previous_hash[HASH_CHARS]) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    sha3_update(&ctx, previous_hash, HASH_CHARS);
    sha3_finalize(&ctx, result);
    // TODO: Fix result if Architecture is BigEndian
}

// TODO: Switch to little endian
void sha3_rand(
        uint64_t out[HASH_UINT64S],
        const unsigned char previous_hash[HASH_CHARS],
        const uint64_t nonce) {
    uint8_t result[HASH_CHARS];
    int i, j;
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    sha3_update(&ctx, previous_hash, HASH_CHARS);
    sha3_update(&ctx, (const uint8_t *) &nonce, sizeof(uint64_t));
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
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    for(int i = 0; i < WIDTH; ++i) {
        sha3_update(&ctx, (const uint8_t *) &(mix[i]), sizeof(uint64_t));
    }
    sha3_update(&ctx, (const uint8_t *) &nonce, sizeof(uint64_t));
    sha3_finalize(&ctx, result);
}

uint32_t cube_mod_safe_prime(const uint32_t x) {
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

// TODO: Test Me
void produce_dag(
        uint64_t *dag,
        const parameters params,
        const unsigned char previous_hash[HASH_CHARS]) {
    sha3_dag((uint8_t *) dag, previous_hash);
    uint32_t picker1 = (uint32_t) dag[0] % SAFE_PRIME,
            picker2, worker1, worker2;
    uint64_t x;
    size_t i;
    int j;
    picker1 = picker1 < 2 ? 2 : picker1;
    for (i = HASH_UINT64S; i < params.dag_size; ++i) {
        picker2 = cube_mod_safe_prime(picker1);
        worker1 = picker1 = cube_mod_safe_prime(picker2);
        x = ((uint64_t) picker2 << 32) | picker1;
        x ^= dag[x % i];
        for (j = 0; j < params.work_factor; ++j) {
            worker2 = cube_mod_safe_prime2(worker1);
            worker1 = cube_mod_safe_prime2(worker2);
            x ^= ((uint64_t) worker2 << 32) + worker1;
        }
        dag[i] = x;
    }
}

static const uint32_t powers_of_three_mod_totient[32] = {
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
            r %= SAFE_PRIME_TOTIENT;
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

uint32_t quick_bbs(const uint32_t power_table[32], const uint64_t p) {
    uint32_t q = three_pow_mod_totient(
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

// TODO: Test Me
uint64_t light_calc_cached(
        const uint64_t *cache,
        const uint32_t power_table[32],
        const parameters params,
        const uint64_t pos) {
    if (pos < params.cache_size)
        return cache[pos];
    else {
        const uint32_t
                picker2 = quick_bbs(power_table, pos + 1 - HASH_UINT64S),
                picker1 = cube_mod_safe_prime(picker2);
        uint64_t x = ((uint64_t) picker2 << 32) | picker1;
        x ^= light_calc_cached(cache, power_table, params, x % pos);
        uint32_t worker1 = picker1, worker2;
        for (int j = 0; j < params.work_factor; ++j) {
            worker2 = cube_mod_safe_prime2(worker1);
            worker1 = cube_mod_safe_prime2(worker2);
            x ^= ((uint64_t) worker2 << 32) + worker1;
        }
        return x;
    }
}

// Todo: Test Me
uint64_t light_calc(
        parameters params,
        const unsigned char previous_hash[HASH_CHARS],
        const uint64_t pos) {

    uint64_t * cache = alloca(sizeof(uint64_t) * params.cache_size);
    size_t original_dag_size = params.dag_size;
    params.dag_size = params.cache_size;
    produce_dag(cache, params, previous_hash);
    params.dag_size = original_dag_size;

    uint32_t seed = (uint32_t) cache[0] % SAFE_PRIME;
    seed = seed < 2 ? 2 : seed;
    uint32_t power_table[32];
    init_power_table_mod_prime(power_table, seed);

    return light_calc_cached(cache, power_table, params, pos);
}

// Todo: Test Me
// TODO: Rename This bears no resemblance at all to the original Hashimoto loop
void hashimoto(
        unsigned char result[HASH_CHARS],
        const uint64_t *dag,
        const parameters params,
        const unsigned char previous_hash[HASH_CHARS],
        const uint64_t nonce) {
    uint64_t rands[HASH_UINT64S];
    const uint64_t m = params.dag_size / WIDTH; // TODO: Force this to be prime
    sha3_rand(rands, previous_hash, nonce);
    uint32_t picker1 = (uint32_t) rands[0] % SAFE_PRIME,
             picker2 = cube_mod_safe_prime(picker1);
    size_t ind = (((size_t) picker2 << 32) | picker1) % m;
    uint64_t mix[WIDTH];
    int i, p;
    for (i = 0; i < WIDTH; ++i) 
        mix[i] = dag[ind + i];
    for (p = 0; p < params.accesses; ++p) {
        picker1 = cube_mod_safe_prime(picker2);
        picker2 = cube_mod_safe_prime(picker1);
        ind = (((size_t) picker2 << 32) | picker1) % m;
        for (i = 0; i < WIDTH; ++i)
            mix[i] ^= dag[ind + i]; // TODO: Try something not associative and commutative
    }
    sha3_mix(result, mix, nonce);
}

// TODO: Test Me
void light_hashimoto_cached(
        unsigned char result[HASH_CHARS],
        const uint64_t *cache,
        const uint32_t power_table[32],
        const parameters params,
        const unsigned char previous_hash[32],
        const uint64_t nonce) {
    uint64_t rands[HASH_UINT64S];
    const uint64_t m = params.dag_size / WIDTH; // TODO: Force this to be prime
    sha3_rand(rands, previous_hash, nonce);
    uint32_t picker1 = (uint32_t) rands[0] % SAFE_PRIME,
            picker2 = cube_mod_safe_prime(picker1);
    size_t ind = (((size_t) picker2 << 32) | picker1) % m;
    uint64_t mix[WIDTH];
    int i, p;
    for (i = 0; i < WIDTH; ++i)
        mix[i] = light_calc_cached(cache, power_table, params, ind + i);
    for (p = 0; p < params.accesses; ++p) {
        picker1 = cube_mod_safe_prime(picker2);
        picker2 = cube_mod_safe_prime(picker1);
        ind = (((size_t) picker2 << 32) | picker1) % m;
        for (i = 0; i < WIDTH; ++i)
            mix[i] ^= light_calc_cached(cache, power_table, params, ind + i); // TODO: Try something not associative and commutative
    }
    sha3_mix(result, mix, nonce);
}

// TODO: Test Me
void light_hashimoto(
        unsigned char result[32],
        parameters params,
        const unsigned char previous_hash[32],
        const uint64_t nonce) {
    const uint64_t original_dag_size = params.dag_size;

    uint64_t *cache = alloca(sizeof(uint64_t) * params.cache_size); // Might be too large for stack
    params.dag_size = params.cache_size;
    produce_dag(cache, params, previous_hash);
    params.dag_size = original_dag_size;

    uint32_t seed = (uint32_t) cache[0] % SAFE_PRIME;
    seed = seed < 2 ? 2 : seed;
    uint32_t power_table[32];
    init_power_table_mod_prime(power_table, seed);
    light_hashimoto_cached(result, cache, power_table, params, previous_hash, nonce);
}