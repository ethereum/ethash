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

BOOST_AUTO_TEST_CASE(light_and_full_client_checks) {
    ethash_params params;
    uint8_t seed[32], previous_hash[32], light_out[32], full_out[32];
    memcpy(seed, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    memcpy(previous_hash, "~~~X~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 32);
    ethash_params_init(&params,0);
    params.cache_size = 1024;
    params.full_size = 1024 * 32;
    ethash_cache cache;
    cache.mem = alloca(params.cache_size);
    ethash_mkcache(&cache, &params, seed);
    void * full_mem = alloca(params.full_size);
    ethash_compute_full_data(full_mem, &params, &cache);

    {
        const std::string
                expected = "cfd4b2e864f54aaf40f38a193077c99a9575145c567ef07cadd0b57c22136bd8c5f92ac1e8cfe6073c5714945c340c5468aaa82f5e93469e8037c634e2d86fa86072107ab022802e3f2eb51516f9b9ee945c7b3f11d353595ebde40a1dc04054b43a33687bf76574c84dee7b926044442e484778322be2becba3b3c770ed412357a0ecc0757da95ed4d07e57a3b77efce8081c7c20821c7849198ad090665046596756adff7a73a5dbcdfaff2e84b06cd4ae253c58e2ebddda7340616d3e3c3a641f60c5e6cb3b5f9cc2768ed437ae83e592a762e9d412cf127881e6697a0027b6b4d4c76fa234e34bc7d934221ceb754a67b9a0d8419162b32f368ef2d7d4b19ee7144451962196cf8d06607d609e0784e7ee3701bf9c1283f4cf4faaa437585d39ab2e3cbfef322ba3814b668790184a70d3bfeb8758034bea6ac817469f8c0592d171ddeaf4c4ac95a116d761d650dd3cca7d7461f15d50b03cd67fa1bcdd2b398ac585f37f854777b96dcd5a3a0b5f70920295db9511c1c4b88c49fab4404dc8b258f5e22d3c4d2288ec2b16b121299b912ca030787a5a4be987747708ab099149c41b088176f2f44f5b4470c484d6ea63d69ee3bda9d0a29a1a4f72e17411088ecbce085342f95d2896c37187d89756c9861d2f53ec7175e9c5975cd40f8516e879ca936ca7dfd41acc3b542e016354e7f4f5c97255907761f6c972c68f0b137d9ee3c13122027f438de8de53b87ebf622b13d24c5fb66588633fd2218dbe0ed59ca7cf46b4522768e4b3323c2ce685c50eec2040fe48bc6bb24fcd82abfcb41965f19b76373730395198b9e52a2bd2875916b7d3fccded247290652fb8067cca1aba67a5fb741dcec7c588c70d4a942277e524e3d28c2c2681d113f81050926f6db80ad5d2fb14f02c074e35a64ac20d3d3409d3296c50cea22b79d4bcd06d65455fa86f90fe2b9d0b07b07324f94739d07b9c115e57ed6443279945bd3c41fb32e998a1c3a94d9dfd42ef633306d7f72bc56d3509386601315cdccd7bbf4df8b61bc7f046a330c03e50d908b12d0c162f7d390dfcde47d2d32573e1418de18b2cec9ec3664264b0043e8ed4fa794e3a3381bbffcc6385567b33a839c79fbb8f0d46e2a264fd21ace9a3dafad31950252d80da310f0841d511e990624f6fad74b514033e71a54bc20d106450456aa7107a99dc2a8aad8d81ade72a8189dfd69baad2e3142b6489bcfb89bbd82bbf8cd71faec08aafa974640c1389c7b2d82fd66d8b00bcd60ee0d863cc35bf1277527e2dd2ea0362834bda4d732415f15fd0455f7e9006ed61129a23cab8488aa72d5a703905cb12dba61887826498a1ffd31be7cfb506faa7732df638b86bbad728ef1cbfad16484f2019dd23362ced06e497154f820afe8972cf5a576ba6af4f1b257334816580a7614ac03c161517",
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