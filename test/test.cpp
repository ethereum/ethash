#include <iomanip>
#include <libethash/blum_blum_shub.h>
#include <libethash/fnv.h>
#include <libethash/nth_prime.h>
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

std::string strToHex(const uint8_t * str, const size_t s) {
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
            expected = 3565965887U,
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
            actual = strToHex(out, 32);
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
            actual = strToHex(out, 64);
    BOOST_REQUIRE_MESSAGE(expected == actual,
            "\nexpected: " << expected.c_str() << "\n"
                    << "actual: " << actual.c_str() << "\n");
}

BOOST_AUTO_TEST_CASE(light_client_checks) {
    ethash_params params;
    uint8_t seed[32], previous_hash[32], ret[32];
    memcpy(seed, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    memcpy(previous_hash, "~~~X~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    ethash_params_init(&params);
    params.cache_size = 128;
    ethash_cache c;
    c.mem = alloca(params.cache_size);
    ethash_mkcache(&c, &params, seed);

    {
        const std::string
                expected = "9ef8038dfc581e5e96fefb18b8cf658f3161badd7818ba643405790c35c9b717468441f7f9b73693c3689e5fe0adb4037f66a38caf47b89093d948456a55fd37c61f74ea9bea0910ce069b6f25f9f0861fc0b6b89b8b1aef55267e730ed7d0ac4485daf189caf470e51d4b5012a7332d6d475c8e6d07258362064955bcc0ea62",
                actual = strToHex((uint8_t const *) c.mem, params.cache_size);

        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected.c_str() << "\n"
                        << "actual: " << actual.c_str() << "\n");
    }
    {
        for (int i = 0; i < 32; ++i)
            BOOST_REQUIRE(c.rng_table[i] % SAFE_PRIME == c.rng_table[i]);
    }
    {
        uint64_t tmp = ((uint64_t *) c.mem)[0] % SAFE_PRIME;
        for (int i = 0; i < 32; ++i) {
            BOOST_REQUIRE_MESSAGE(tmp % SAFE_PRIME == c.rng_table[i],
                    "\nexpected: " << tmp << "\n"
                            << "actual: " << c.rng_table[i] << "\n");
            tmp *= tmp;
            tmp %= SAFE_PRIME;
        }
    }

    {
        ethash_light(ret, &c, &params, previous_hash, 5);
        const std::string
                expected = "9ef8038dfc581e5e96fefb18b8cf658f3161badd7818ba643405790c35c9b717468441f7f9b73693c3689e5fe0adb4037f66a38caf47b89093d948456a55fd37c61f74ea9bea0910ce069b6f25f9f0861fc0b6b89b8b1aef55267e730ed7d0ac4485daf189caf470e51d4b5012a7332d6d475c8e6d07258362064955bcc0ea62",
                actual = strToHex(ret, 32);
        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected.c_str() << "\n"
                        << "actual: " << actual.c_str() << "\n");
    }
}

BOOST_AUTO_TEST_CASE(nth_prime_check) {
    {
        const uint32_t
                expected = 2,
                actual = nth_prime(0);
        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }

    {
        const uint32_t
                expected = 7919,
                actual = nth_prime(1000-1);
        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }

    {
        const uint32_t
                expected = 262147,
                actual = nth_prime(23001-1);
        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }

    {
        const uint32_t
                expected = 274529,
                actual = nth_prime(24000-1);
        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
}

BOOST_AUTO_TEST_CASE(check_hash_less_than_difficulty_check) {
    {
        uint8_t hash[32], difficulty[32];
        memcpy(hash, "11111111111111111111111111111111", 32);
        memcpy(difficulty, "22222222222222222222222222222222", 32);
        BOOST_REQUIRE(check_hash_less_than_difficulty(hash, difficulty));
    }
    {
        uint8_t hash[32], difficulty[32];
        memcpy(hash, "11111111111111111111111111111113", 32);
        memcpy(difficulty, "22222222222222222222222222222222", 32);
        BOOST_REQUIRE(! check_hash_less_than_difficulty(hash, difficulty));
    }
    {
        uint8_t hash[32], difficulty[32];
        memcpy(hash, "22222222222222222222222222222222", 32);
        memcpy(difficulty, "22222222222222222222222222222222", 32);
        BOOST_REQUIRE(! check_hash_less_than_difficulty(hash, difficulty));
    }
    {
        uint8_t hash[32], difficulty[32];
        memcpy(hash, "12222222222222222222222222222222", 32);
        memcpy(difficulty, "22222222222222222222222222222222", 32);
        BOOST_REQUIRE(check_hash_less_than_difficulty(hash, difficulty));
    }
}