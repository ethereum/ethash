#include <stdlib.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif
#include "internal.h"
#include "sha3.h"
#include "blum_blum_shub.h"

void sha3_dag(uint8_t result[HASH_CHARS], const uint8_t previous_hash[HASH_CHARS]) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    sha3_update(&ctx, previous_hash, HASH_CHARS);
    sha3_finalize(&ctx, result);
    // TODO: Fix result if Architecture is BigEndian
}

void sha3_rand(
        uint64_t out[HASH_UINT64S],
        const uint8_t previous_hash[HASH_CHARS],
        const uint64_t nonce) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    sha3_update(&ctx, previous_hash, HASH_CHARS);
    sha3_update(&ctx, (const uint8_t *) &nonce, sizeof(uint64_t));
    sha3_finalize(&ctx, (uint8_t *) out);
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
    // TODO: Fix result if Architecture is BigEndian
}



// TODO: Test Me
void produce_dag(
        uint64_t *dag,
        const parameters params,
        const uint8_t previous_hash[HASH_CHARS]) {
    sha3_dag((uint8_t *) dag, previous_hash);
    for (uint64_t i = HASH_UINT64S; i < WIDTH; i += HASH_UINT64S)
        sha3_dag((uint8_t *) &(dag[i]), (uint8_t *) &(dag[i - HASH_UINT64S]));

    uint32_t picker1 = (uint32_t) (dag[0] % SAFE_PRIME),
            picker2, worker1, worker2;
    uint64_t x;

    // Clamp picker1 in [2,SAFE_PRIME - 2]
    picker1 = picker1 < 2 ? 2 :
            picker1 == SAFE_PRIME - 1 ? SAFE_PRIME - 2 : picker1;

    for (uint64_t i = 1; i < params.dag_size; ++i) {
        picker2 = cube_mod_safe_prime1(picker1);
        picker1 = cube_mod_safe_prime1(picker2);
        x = ((uint64_t) picker2 << 32) | picker1;
        x ^= dag[(x % i) * WIDTH];
        dag[i * WIDTH] = x;
        worker1 = (uint32_t) (x % SAFE_PRIME2);
        // Clamp worker1 in [2, SAFE_PRIME-2]
        worker1 = worker1 < 2 ? 2 :
                worker1 == SAFE_PRIME2 - 1 ? SAFE_PRIME2 - 2 : worker1;

        for (uint64_t j = 1 ; j < WIDTH ; j++) {
            worker2 = cube_mod_safe_prime2(worker1);
            worker1 = cube_mod_safe_prime2(worker2);
            dag[i * WIDTH + j] = dag[i * WIDTH + j - 1] ^ (((uint64_t) worker1 << 32) | worker2);
        }
    }
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
                picker1 = cube_mod_safe_prime1(picker2);
        uint64_t x = ((uint64_t) picker2 << 32) | picker1;
        x ^= light_calc_cached(cache, power_table, params, x % pos);
        return x;
    }
}

// Todo: Test Me
uint64_t light_calc(
        parameters params,
        const uint8_t previous_hash[HASH_CHARS],
        const uint64_t pos) {

    uint64_t * cache = alloca(sizeof(uint64_t) * params.cache_size);
    uint64_t original_dag_size = params.dag_size;
    params.dag_size = params.cache_size;
    produce_dag(cache, params, previous_hash);
    params.dag_size = original_dag_size;

    uint32_t seed = (uint32_t) (cache[0] % SAFE_PRIME);
    seed = seed < 2 ? 2 : seed;
    uint32_t power_table[32];
    init_power_table_mod_prime1(power_table, seed);

    return light_calc_cached(cache, power_table, params, pos);
}

// Todo: Test Me
// TODO: Rename This bears no resemblance at all to the original Hashimoto loop
void hashimoto(
        uint8_t result[HASH_CHARS],
        const uint64_t *dag,
        const parameters params,
        const uint8_t previous_hash[HASH_CHARS],
        const uint64_t nonce) {
    uint64_t rands[HASH_UINT64S];
    const uint64_t m = params.dag_size / WIDTH; // TODO: Force this to be prime
    sha3_rand(rands, previous_hash, nonce);
    uint32_t picker1 = (uint32_t) rands[0] % SAFE_PRIME,
             picker2 = cube_mod_safe_prime1(picker1);
    uint64_t ind = (((uint64_t) picker2 << 32) | picker1) % m;
    uint64_t mix[WIDTH];
    int i, p;
    for (i = 0; i < WIDTH; ++i) 
        mix[i] = dag[ind + i];
    for (p = 0; p < params.accesses; ++p) {
        picker1 = cube_mod_safe_prime1(picker2);
        picker2 = cube_mod_safe_prime1(picker1);
        ind = (((uint64_t) picker2 << 32) | picker1) % m;
        for (i = 0; i < WIDTH; ++i)
            mix[i] ^= dag[ind + i]; // TODO: Try something not associative and commutative
    }
    sha3_mix(result, mix, nonce);
}

// TODO: Test Me
void light_hashimoto_cached(
        uint8_t result[HASH_CHARS],
        const uint64_t *cache,
        const uint32_t power_table[32],
        const parameters params,
        const uint8_t previous_hash[32],
        const uint64_t nonce) {
//    uint64_t rands[HASH_UINT64S];
//    const uint64_t m = params.dag_size / WIDTH; // TODO: Force this to be prime
//    sha3_rand(rands, previous_hash, nonce);
//    uint32_t picker1 = (uint32_t) rands[0] % SAFE_PRIME;
//    uint32_t picker2 = cube_mod_safe_prime(picker1);
//    uint64_t ind = (((uint64_t) picker2 << 32) | picker1) % m;
//    uint64_t x, worker1, worker2, mix[WIDTH];
//    int i, p;
//    for (i = 0; i < WIDTH; ++i)
//        mix[i] = light_calc_cached(cache, power_table, params, ind + i);
//    for (p = 0; p < params.accesses; ++p) {
//        picker1 = cube_mod_safe_prime(picker2);
//        picker2 = cube_mod_safe_prime(picker1);
//        ind = (((uint64_t) picker2 << 32) | picker1) % m;
//        x = ;
//        worker1 = (uint32_t) (x % SAFE_PRIME2);
//        // Clamp worker1 in [2, SAFE_PRIME-2]
//        worker1 = worker1 < 2 ? 2 :
//                worker1 == SAFE_PRIME2 - 1 ? SAFE_PRIME2 - 2 : worker1;
//
//        for (uint64_t j = 1 ; j < WIDTH ; j++) {
//            worker2 = cube_mod_safe_prime2(worker1);
//            worker1 = cube_mod_safe_prime2(worker2);
//            dag[i * WIDTH + j] = dag[i * WIDTH + j - 1] ^ (((uint64_t) worker1 << 32) | worker2);
//        }
//
//    }
//    sha3_mix(result, mix, nonce);
}

// TODO: Test Me
void light_hashimoto(
        uint8_t result[32],
        parameters params,
        const uint8_t previous_hash[32],
        const uint64_t nonce) {
    const uint64_t original_dag_size = params.dag_size;

    uint64_t *cache = alloca(sizeof(uint64_t) * params.cache_size); // Might be too large for stack
    params.dag_size = params.cache_size;
    produce_dag(cache, params, previous_hash);
    params.dag_size = original_dag_size;

    uint32_t seed = (uint32_t) cache[0] % SAFE_PRIME;
    seed = seed < 2 ? 2 : seed;
    uint32_t power_table[32];
    init_power_table_mod_prime1(power_table, seed);
    light_hashimoto_cached(result, cache, power_table, params, previous_hash, nonce);
}