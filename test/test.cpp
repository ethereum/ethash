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
    const uint32_t expected_cache_size = 33554368;
    BOOST_REQUIRE_MESSAGE(params.full_size  == expected_full_size,
            "\nexpected: " << expected_cache_size << "\n"
                    << "actual: " << params.full_size << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size  == expected_cache_size,
            "\nexpected: " << expected_cache_size << "\n"
                    << "actual: " << params.cache_size << "\n");
}

BOOST_AUTO_TEST_CASE(ethash_params_init_3_year_check) {
    ethash_params params;
    ethash_params_init(&params, 7884*1000);
    const uint64_t three_year_dag_size = 2*DAGSIZE_BYTES_INIT;
    BOOST_REQUIRE_MESSAGE(params.full_size  < three_year_dag_size,
            "\nfull size: " << params.full_size << "\n"
                    << "should be less than or equal to: " << three_year_dag_size << "\n");
    BOOST_REQUIRE_MESSAGE(params.full_size + DAGSIZE_BYTES_INIT / 3 > three_year_dag_size,
            "\nfull size + DAGSIZE_BYTES_INIT / 3: " << params.full_size + DAGSIZE_BYTES_INIT / 3 << "\n"
                    << "should be greater than or equal to: " << three_year_dag_size << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size < three_year_dag_size / 32,
            "\ncache size: " << params.cache_size << "\n"
                    << "actual: " << three_year_dag_size / 32 << "\n");
    BOOST_REQUIRE_MESSAGE(params.cache_size + DAGSIZE_BYTES_INIT / 3 / 32 > three_year_dag_size / 32 ,
            "\ncache size + DAGSIZE_BYTES_INIT / 3 / 32: " << params.cache_size + DAGSIZE_BYTES_INIT / 3 / 32 << "\n"
                    << "actual: " << three_year_dag_size / 32 << "\n");
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
                expected = "933f4e31924be948a03b5b781b7d2e5f5a719e5b61fc721a1354521d5b78e0250b1467feca786b254ddc4475f210a29da72fce00140c22ed9a836ddc4e9d564d00a673421a8c82e056565512aa4552350dff1eed248949390578cadf0692c9756014d3614702ab32d4f30e908a89e620ad3038183fbb80a4d600d410c54ec7061132b0ae1b0ff2c290aa7590d36ef381c9fd69c0e08441b728059c9776841c39368742acaa994867a211fc93701f92eceabec6db26e6cd6cf1868d3cee52e87dd7969b0aeca110a2468710a859419974046121953811f983de249eccad48899dfd82be638966be19d27c286ac506e68fb87f86223b9403651d04a8e122c30f02f97d6f4205eb6e80b21cd1db3d52eec2257a09f4e76135f16ff985564f60a96a7de52521cd5988425244d88522dd3cd920c0f4994bf35fa96812d36d2907365bf656b3a36e2c9a75f7ec69ad915a27ccbb85526609b1de45913880dd737388fc102dc2f498237bc215f68ee3d78efacff99a771e34651f473598385d9afd5875a8620b2ebeca41fbc773bb837b5e724d6eb2de570d99858df0d7d97067fb8103b21757873b735097b35d3bea8fd1c359a9e8a63c1540c76c9784cf8d975e995ca8620b2ebeca41fbc773bb837b5e724d6eb2de570d99858df0d7d97067fb8103b21757873b735097b35d3bea8fd1c359a9e8a63c1540c76c9784cf8d975e995cfe32df43abded04643f87e3aa526c116f38efaf8e0a77b5634cffc67504b60c4a8169e4e6a21860b60fedfa7fe4bc6694c9ff9ca633dd2370116aa239819817f00598cadd08fb97ffd5832f1120e15474327e4d077ea4a7e1aba694bde022471b03b1e3fc6924a077c001362043b8d76d1837a91b157c7fac94df52c59d0f0b5bf534b40e40459ff8dc0d6a057a43d803a50122f38edcff71cafd59dbb737ec70589ee87b00b897c86648a519d6955fa200c6834dd0757ec23374098ea3edc487d654cc2255b52c97617275e463848bf89eccaeb4a5a9c588ae6c6764fa69416c13ad8cd209da9f724273bb44c9a34f926228f0dc1a097bd6fa1d4d0b19d863020a5256284581717e02269e339ebbfcde86ee22b18dd4d6026de16b59baa2dbac818d474cd6b7abc05e844edae9ed404fb77d2819df6267639861a02435ba443d2837b725b933025a55785774490431a1aa6ba9af3155e4fc619ded36190dffbc50b36cb7b57fb5c6aeb0bac69d829757136e0f07df4925a7c5e3af1e2134d57d80b714ec24d6754d713507ba666bad33dab202041229d5af6f30ea1443a673e5b6bc86aa961879f6f5da478900def77e87fbd0979385419feee8442326bc76a02bf53b49e6619d52094df401f4ce6d5b1bbe59a2d15243189557fb04d76cb3d8ee4c9def3d4b4e0cde01e0d43bdd7318ecf7df17b3effa812b8ef32c4dcbb04",
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
                expected = "88fd2b9b0a4fcdcbadebd87afe1b37418c3b9d806f5823d48d8b9916c9c94f15d1c119520999d189bf969181ecb51b1bdf3ae6f31782e5f71618a804b6daa438";
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