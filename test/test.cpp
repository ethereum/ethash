#include <iomanip>
#include <libdaggerhashimoto/daggerhashimoto.h>

#define BOOST_TEST_MODULE Daggerhashimoto
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>


std::string hashToHex(const uint8_t str[HASH_CHARS]) {
    std::ostringstream ret;

    for (int i = 0; i < HASH_CHARS; ++i)
        ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int) str[i];

    return ret.str();
}

BOOST_AUTO_TEST_CASE(sha3_dag_check) {
    {
        uint8_t input[HASH_CHARS], result[HASH_CHARS];
        memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
        sha3_dag(result, input);

        const std::string
                expected = "2b5ddf6f4d21c23de216f44d5e4bdc68e044b71897837ea74c83908be7037cd7",
                actual = hashToHex(result);

        BOOST_REQUIRE_MESSAGE(expected == actual,
                "\nexpected: " << expected.c_str() << "\n"
                        << "actual: " << actual.c_str() << "\n");
    }
    {
        uint8_t input[HASH_CHARS];
        memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
        uint64_t actual[4],
                expected[4] = {
                4450155998268579115U,
                7555997143227700962U,
                12069228736377472224U,
                15527289908280460108U};
        sha3_dag((uint8_t *) actual, input);
        for (int i = 0; i < 4; i++) {
            BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                    "\nexpected: " << expected[i] << "\n"
                            << "actual: " << actual[i] << "\n");
        }
    }
}

BOOST_AUTO_TEST_CASE(sha3_rand_check) {
    {
        uint8_t input[HASH_CHARS];
        memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
        const uint64_t nonce = 0x7E7E7E7E7E7E7E7EU,
                expected[HASH_UINT64S] = {
                6154632431212981991U,
                520511268698457443U,
                6437342779080611984U,
                14985183232610838775U};
        uint64_t actual[HASH_UINT64S];
        sha3_rand(actual, input, nonce);
        for (int i = 0; i < HASH_UINT64S; i++) {
            BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                    "\nexpected: " << expected[i] << "\n"
                            << "actual: " << actual[i] << "\n");
        }
    }
    {
        uint8_t input[HASH_CHARS];
        memcpy(input, "~~~~~~H~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
        const uint64_t nonce = 0x7E597E7E7E7E7E7EU,
                expected[HASH_UINT64S] = {
                12131474505527047766U,
                6365449550867347897U,
                14172837750324433329U,
                5787924029805105676U};
        uint64_t actual[HASH_UINT64S];
        sha3_rand(actual, input, nonce);
        for (int i = 0; i < HASH_UINT64S; i++) {
            BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                    "\nexpected: " << expected[i] << "\n"
                            << "actual: " << actual[i] << "\n");
        }
    }
}



BOOST_AUTO_TEST_CASE(cube_mod_safe_prime_check) {
    const uint32_t expected = 4294966087U,
            actual = cube_mod_safe_prime(4294967077U);

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
                actual = three_pow_mod_totient(0);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 3,
                actual = three_pow_mod_totient(1);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 9,
                actual = three_pow_mod_totient(2);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 27,
                actual = three_pow_mod_totient(3);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 3748161571U,
                actual = three_pow_mod_totient(4294967295U);

        BOOST_REQUIRE_MESSAGE(actual == expected,
                "\nexpected: " << expected << "\n"
                        << "actual: " << actual << "\n");
    }
    {
        const uint32_t
                expected = 3106101787U,
                actual = three_pow_mod_totient(2147483648U);

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
    init_power_table_mod_prime(actual, 1758178831U);
    for(int i = 0; i < 32 ; ++i)
        BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                "\nexpected: " << expected[i] << "\n"
                        << "actual: " << actual[i] << "\n");
}

BOOST_AUTO_TEST_CASE(quick_bbs_check) {
    uint32_t table[32];
    init_power_table_mod_prime(table, 1799198831U);
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