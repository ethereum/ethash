#include <iomanip>
#include <libethash/blum_blum_shub.h>
#include <libethash/fnv.h>
#include <libethash/ethash.h>
#include <libethash/internal.h>

#ifdef WITH_CRYPTOPP
#include <libethash/sha3_cryptopp.h>
#else
#include <libethash/sha3.h>
#endif // WITH_CRYPTOPP

#define BOOST_TEST_MODULE Daggerhashimoto
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <libethash/ethash.h>
#include <iostream>

std::string bytesToHexString(const uint8_t *str, const size_t s) {
    std::ostringstream ret;

    for (int i = 0; i < s; ++i)
        ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int) str[i];

    return ret.str();
}

BOOST_AUTO_TEST_CASE(cube_mod_safe_prime1_check) {
    const uint32_t expected = 4294966087U,
            actual = cube_mod_safe_prime1(4294967077U);

    BOOST_REQUIRE_MESSAGE(actual == expected,
            "\nexpected: " << expected << "\n"
                    << "actual: " << actual << "\n");

}

BOOST_AUTO_TEST_CASE(cube_mod_safe_prime2_check) {
    const uint32_t
            expected = 1728000000U,
            actual = cube_mod_safe_prime2(4294964987U);

    BOOST_REQUIRE_MESSAGE(actual == expected,
            "\nexpected: " << expected << "\n"
                    << "actual: " << actual << "\n");
}

BOOST_AUTO_TEST_CASE(three_pow_mod_totient_check) {
    {
        const uint32_t
                expected = 1,
                actual = three_pow_mod_totient1(0);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 3,
                actual = three_pow_mod_totient1(1);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 9,
                actual = three_pow_mod_totient1(2);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 27,
                actual = three_pow_mod_totient1(3);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 3748161571U,
                actual = three_pow_mod_totient1(4294967295U);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 3106101787U,
                actual = three_pow_mod_totient1(2147483648U);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
}

BOOST_AUTO_TEST_CASE(init_power_table_mod_prime_check) {
    const uint32_t expected[32] = {
            1758178831, 3087151933U, 2181741089U, 2215739027U, 1172752426U,
            2166186118U, 952137455U, 1932908534U, 2055989032U, 3668501270U,
            3361953768U, 2864264791U, 346776217U, 589953143U, 46265863U,
            87348622U, 368498995U, 237438963U, 2748204571U, 3669701545U,
            3941733513U, 1373024902U, 477501137U, 1476916330U, 3722281540U,
            2393041984U, 3169721271U, 680334287U, 3255565205U, 2133070878U,
            4212360994U, 202306615U};
    uint32_t actual[32];
    init_power_table_mod_prime1(actual, 1758178831U);
    for(int i = 0; i < 32 ; ++i)
        BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                "\nexpected: " << expected[i] << "\n"
                        << "actual: " << actual[i] << "\n");
}

BOOST_AUTO_TEST_CASE(quick_bbs_check) {
    uint32_t table[32];
    init_power_table_mod_prime1(table, 1799198831U);
    {
        const uint32_t
                expected = 1799198831U,
                actual = quick_bbs(table,0);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 2685204534U,
                actual = quick_bbs(table,1);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 542784404U,
                actual = quick_bbs(table,0xFFFFFFFF);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 4097726076U,
                actual = quick_bbs(table,0xFFFFFFFFFFFFFFFF);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
}

BOOST_AUTO_TEST_CASE(fnv_hash_check) {
    uint32_t x = 1235U;
    const uint32_t
            y = 9999999U,
            expected = (FNV_PRIME * x) ^ y;

    x = fnv_hash(x, y);

    BOOST_REQUIRE_MESSAGE(x == expected,
            "\nexpected: " << expected << "\n"
                    << "actual: " << x << "\n");

}

BOOST_AUTO_TEST_CASE(SHA256_check) {
    uint8_t input[32], out[32];
    memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    SHA3_256(out, input, 32);
    const std::string
            expected = "2b5ddf6f4d21c23de216f44d5e4bdc68e044b71897837ea74c83908be7037cd7",
            actual = bytesToHexString(out, 32);
    BOOST_REQUIRE_MESSAGE(expected == actual,
            "\nexpected: " << expected.c_str() << "\n"
                    << "actual: " << actual.c_str() << "\n");
}

BOOST_AUTO_TEST_CASE(SHA512_check) {
    uint8_t input[64], out[64];
    memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 64);
    SHA3_512(out, input, 64);
    const std::string
            expected = "0be8a1d334b4655fe58c6b38789f984bb13225684e86b20517a55ab2386c7b61c306f25e0627c60064cecd6d80cd67a82b3890bd1289b7ceb473aad56a359405",
            actual = bytesToHexString(out, 64);
    BOOST_REQUIRE_MESSAGE(expected == actual,
            "\nexpected: " << expected.c_str() << "\n"
                    << "actual: " << actual.c_str() << "\n");
}

BOOST_AUTO_TEST_CASE(ethash_params_init_genesis_check) {
    ethash_params params;
    ethash_params_init(&params, 0);
    BOOST_REQUIRE_MESSAGE(params.full_size  < DAGSIZE_BYTES_INIT,
            "\nfull size: " << params.full_size << "\n"
                    << "should be less than or equal to: " << DAGSIZE_BYTES_INIT << "\n");
    BOOST_REQUIRE_MESSAGE(params.full_size + 8*MIX_BYTES >= DAGSIZE_BYTES_INIT,
            "\nfull size + 8*MIX_BYTES: " << params.full_size + 8*MIX_BYTES << "\n"
                    << "should be greater than or equal to: " << DAGSIZE_BYTES_INIT << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size < DAGSIZE_BYTES_INIT / 32,
            "\ncache size: " << params.cache_size << "\n"
                    << "should be less than or equal to: " << DAGSIZE_BYTES_INIT / 32 << "\n");
}

BOOST_AUTO_TEST_CASE(ethash_params_init_genesis_calcifide_check) {
    ethash_params params;
    ethash_params_init(&params, 0);
    const uint32_t expected_full_size = 1073721344;
    const uint32_t expected_cache_size = 1048384;
    BOOST_REQUIRE_MESSAGE(params.full_size  == expected_full_size,
            "\nexpected: " << expected_cache_size << "\n"
                    << "actual: " << params.full_size << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size  == expected_cache_size,
            "\nexpected: " << expected_cache_size << "\n"
                    << "actual: " << params.cache_size << "\n");
}

BOOST_AUTO_TEST_CASE(ethash_params_init_check) {
    ethash_params params;
    ethash_params_init(&params, 1971000);
    const uint64_t nine_month_size = (uint64_t) 8*DAGSIZE_BYTES_INIT;
    BOOST_REQUIRE_MESSAGE(params.full_size  < nine_month_size,
            "\nfull size: " << params.full_size << "\n"
                    << "should be less than or equal to: " << nine_month_size << "\n");
    BOOST_REQUIRE_MESSAGE(params.full_size + DAGSIZE_BYTES_INIT / 4 > nine_month_size,
            "\nfull size + DAGSIZE_BYTES_INIT / 4: " << params.full_size + DAGSIZE_BYTES_INIT / 4 << "\n"
                    << "should be greater than or equal to: " << nine_month_size << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size < nine_month_size / 1024,
            "\ncache size: " << params.cache_size << "\n"
                    << "actual: " << nine_month_size / 1024 << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size + DAGSIZE_BYTES_INIT / 4 / 1024 > nine_month_size / 1024 ,
            "\ncache size + DAGSIZE_BYTES_INIT / 4 / 1024: " << params.cache_size + DAGSIZE_BYTES_INIT / 4 / 1024 << "\n"
                    << "actual: " << nine_month_size / 32 << "\n");
}

BOOST_AUTO_TEST_CASE(light_and_full_client_checks) {
    ethash_params params;
    uint8_t seed[32], previous_hash[32], light_out[32], full_out[32];
    memcpy(seed, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    memcpy(previous_hash, "~~~X~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    ethash_params_init(&params, 0);
    params.cache_size = 1024;
    params.full_size = 1024 * 32;
    ethash_cache cache;
    cache.mem = alloca(params.cache_size);
    ethash_mkcache(&cache, &params, seed);
    node * full_mem = (node *) alloca(params.full_size);
    ethash_compute_full_data(full_mem, &params, &cache);

    {
        const std::string
                expected = "2da2b506f21070e1143d908e867962486d6b0a02e31d468fd5e3a7143aafa76a14201f63374314e2a6aaf84ad2eb57105dea3378378965a1b3873453bb2b78f9a8620b2ebeca41fbc773bb837b5e724d6eb2de570d99858df0d7d97067fb8103b21757873b735097b35d3bea8fd1c359a9e8a63c1540c76c9784cf8d975e995ca8620b2ebeca41fbc773bb837b5e724d6eb2de570d99858df0d7d97067fb8103b21757873b735097b35d3bea8fd1c359a9e8a63c1540c76c9784cf8d975e995ca8620b2ebeca41fbc773bb837b5e724d6eb2de570d99858df0d7d97067fb8103b21757873b735097b35d3bea8fd1c359a9e8a63c1540c76c9784cf8d975e995c259440b89fa3481c2c33171477c305c8e1e421f8d8f6d59585449d0034f3e421808d8da6bbd0b6378f567647cc6c4ba6c434592b198ad444e7284905b7c6adaf70bf43ec2daa7bd5e8951aa609ab472c124cf9eba3d38cff5091dc3f58409edcc386c743c3bd66f92408796ee1e82dd149eaefbf52b00ce33014a6eb3e50625413b072a58bc01da28262f42cbe4f87d4abc2bf287d15618405a1fe4e386fcdafbb171064bd99901d8f81dd6789396ce5e364ac944bbbd75a7827291c70b42d26385910cd53ca535ab29433dd5c5714d26e0dce95514c5ef866329c12e958097e84462197c2b32087849dab33e88b11da61d52f9dbc0b92cc61f742c07dbbf751c49d7678624ee60dfbe62e5e8c47a03d8247643f3d16ad8c8e663953bcda1f59d7e2d4a9bf0768e789432212621967a8f41121ad1df6ae1fa78782530695414c6213942865b2730375019105cae91a4c17a558d4b63059661d9f108362143107babe0b848de412e4da59168cce82bfbff3c99e022dd6ac1e559db991f2e3f7bb910cefd173e65ed00a8d5d416534e2c8416ff23977dbf3eb7180b75c71580d08ce95efeb9b0afe904ea12285a392aff0c8561ff79fca67f694a62b9e52377485c57cc3598d84cac0a9d27960de0cc31ff9bbfe455acaa62c8aa5d2cce96f345da9afe843d258a99c4eaf3650fc62efd81c7b81cd0d534d2d71eeda7a6e315d540b4473c80f8730037dc2ae3e47b986240cfc65ccc565f0d8cde0bc68a57e39a271dda57440b3598bee19f799611d25731a96b5dbbbefdff6f4f656161462633030d62560ea4e9c161cf78fc96a2ca5aaa32453a6c5dea206f766244e8c9d9a8dc61185ce37f1fc804459c5f07434f8ecb34141b8dcae7eae704c950b55556c5f40140c3714b45eddb02637513268778cbf937a33e4e33183685f9deb31ef54e90161e76d969587dd782eaa94e289420e7c2ee908517f5893a26fdb5873d68f92d118d4bcf98d7a4916794d6ab290045e30f9ea00ca547c584b8482b0331ba1539a0f2714fddc3a0b06b0cfbb6a607b8339c39bcfd6640b1f653e9d70ef6c985b",
                actual = bytesToHexString((uint8_t const *) cache.mem, params.cache_size);

        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected.c_str() << "\n"
                        << "actual: " << actual.c_str() << "\n");
    }

    {
        for (int i = 0; i < 32; ++i)
            BOOST_REQUIRE(cache.rng_table[i] % SAFE_PRIME == cache.rng_table[i]);
    }

    {
        uint64_t tmp = make_seed1(((node *) cache.mem)[0].words[0]);
        for (int i = 0; i < 32; ++i) {
            BOOST_REQUIRE_MESSAGE(tmp % SAFE_PRIME == cache.rng_table[i],
                    "\npower: " << i << "\n"
                            << "expected: " << tmp << "\n"
                            << "actual: " << cache.rng_table[i] << "\n");
            tmp *= tmp;
            tmp %= SAFE_PRIME;
        }
    }

    {
        uint64_t tmp = make_seed1(((node *) cache.mem)[0].words[0]);
        for (int i = 0; i < 32; ++i) {
            BOOST_REQUIRE_MESSAGE(tmp % SAFE_PRIME == cache.rng_table[i],
                    "\npower: " << i << "\n"
                            << "expected: " << tmp << "\n"
                            << "actual: " << cache.rng_table[i] << "\n");
            tmp *= tmp;
            tmp %= SAFE_PRIME;
        }
    }

    {
        node node;
        ethash_calculate_dag_item(&node, 0, &params, &cache);
        const std::string
                actual = bytesToHexString((uint8_t const *) &node, sizeof(node)),
                expected = "62580450c02505fbc7ac940c7fd6019402babcec255db94c9c7fac62caeeffc0be7a5cc1cd42e0298b8dc00b9ad94126952b747bf10cb1eaed3ae98c2ba0ea8b";
        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\n" << "expected: " << expected.c_str() << "\n"
                        << "actual: " << actual.c_str() << "\n");
    }

    {
        for (int i = 0 ; i < params.full_size / sizeof(node) ; ++i ) {
            for (uint32_t j = 0; j < 32; ++j) {
                node expected_node;
                ethash_calculate_dag_item(&expected_node, j, &params, &cache);
                const std::string
                        actual = bytesToHexString((uint8_t const *) &(full_mem[j]), sizeof(node)),
                        expected = bytesToHexString((uint8_t const *) &expected_node, sizeof(node));
                BOOST_REQUIRE_MESSAGE(actual == expected,
                        "\ni: " << j << "\n"
                                << "expected: " << expected.c_str() << "\n"
                                << "actual: " << actual.c_str() << "\n");
            }
        }
    }

    {
        ethash_full(full_out, full_mem, &params, previous_hash, 0x7c7c597c);
        ethash_light(light_out, &cache, &params, previous_hash, 0x7c7c597c);
        const std::string
                light_string = bytesToHexString(light_out, 32),
                full_string = bytesToHexString(full_out, 32);
        BOOST_REQUIRE_MESSAGE(light_string == full_string,
                "\nlight: " << light_string.c_str() << "\n"
                        << "full: " << full_string.c_str() << "\n");
    }
    {
        ethash_full(full_out, full_mem, &params, previous_hash, 5);
        ethash_light(light_out, &cache, &params, previous_hash, 5);
        const std::string
                light_string = bytesToHexString(light_out, 32),
                full_string = bytesToHexString(full_out, 32);
        BOOST_REQUIRE_MESSAGE(light_string == full_string,
                "\nlight: " << light_string.c_str() << "\n"
                        << "full: " << full_string.c_str() << "\n");
    }
}

BOOST_AUTO_TEST_CASE(ethash_check_difficulty_check) {
    uint8_t hash[32], target[32];
    memset(hash, 0, 32);
    memset(target, 0, 32);

    memcpy(hash, "11111111111111111111111111111111", 32);
    memcpy(target, "22222222222222222222222222222222", 32);
    BOOST_REQUIRE_MESSAGE(
            ethash_check_difficulty(hash, target),
            "\nexpected \"" << hash << "\" to have less difficulty than \"" << target << "\"\n");
    BOOST_REQUIRE_MESSAGE(
            !ethash_check_difficulty(hash, hash),
            "\nexpected \"" << hash << "\" to have the same difficulty as \"" << hash << "\"\n");
    memcpy(target, "11111111111111111111111111111112", 32);
    BOOST_REQUIRE_MESSAGE(
            ethash_check_difficulty(hash, target),
            "\nexpected \"" << hash << "\" to have less difficulty than \"" << target << "\"\n");
    memcpy(target, "11111111111111111111111111111110", 32);
    BOOST_REQUIRE_MESSAGE(
            !ethash_check_difficulty(hash, target),
            "\nexpected \"" << hash << "\" to have more difficulty than \"" << target << "\"\n");
}