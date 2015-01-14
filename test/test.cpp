// -*-c++-*-
/*
  This file is part of cpp-ethereum.

  cpp-ethereum is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  cpp-ethereum is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file test.cpp
* @author Matthew Wampler-Doty <matt@w-d.org>
* @date 2014
*/

#include <libdaggerhashimoto/daggerhashimoto.h>


#define BOOST_TEST_MODULE Daggerhashimoto
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <iomanip>


std::string hashToHex(const uint8_t str[HASH_CHARS]) {
    std::ostringstream ret;

    for (int i = 0; i < HASH_CHARS; ++i)
        ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int) str[i];

    return ret.str();
}

BOOST_AUTO_TEST_CASE(sha3_1_check) {
    uint8_t input[HASH_CHARS], result[HASH_CHARS];
    memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
    sha3_1(result, input);

    const std::string
            expected = "2b5ddf6f4d21c23de216f44d5e4bdc68e044b71897837ea74c83908be7037cd7",
            actual = hashToHex(result);

    BOOST_REQUIRE_MESSAGE(expected == actual,
            "\nexpected: " << expected.c_str() << "\n"
                    << "actual: " << actual.c_str() << "\n");
}

BOOST_AUTO_TEST_CASE(sha3_dag_check) {
    uint8_t input[HASH_CHARS];
    memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
    uint64_t actual[4],
            expected[4] = {
            3124899385593414205U,
            16291477315191037032U,
            16160242679161257639U,
            5513409299382238423U};
    sha3_dag(actual, input);
    for (int i = 0; i < 4; i++) {

        BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                "\nexpected: " << expected[i] << "\n"
                        << "actual: " << actual[i] << "\n");
    }
}

BOOST_AUTO_TEST_CASE(uint64str_check) {
    uint8_t expected_[8], actual_[8];
    memcpy(expected_, "~~~~~~~~", 8);
    uint64str(actual_, 0x7E7E7E7E7E7E7E7EU);
    const std::string expected((char *) expected_, 8), actual((char *) actual_, 8);
    BOOST_REQUIRE_MESSAGE(actual == expected,
            "\nexpected: " << expected.c_str() << "\n"
                    << "actual: " << actual.c_str() << "\n");
}

BOOST_AUTO_TEST_CASE(sha3_nonce_check) {
    uint8_t input[HASH_CHARS];
    memcpy(input, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", HASH_CHARS);
    const uint64_t nonce = 0x7E7E7E7E7E7E7E7EU,
            expected[HASH_UINT64S] = {
            16676420855326402901U,
            7135211131009382663U,
            10419225811852285529U,
            17845768961284699855U};
    uint64_t actual[HASH_UINT64S];
    sha3_nonce(actual, input, nonce);
    for (int i = 0; i < HASH_UINT64S; i++) {
        BOOST_REQUIRE_MESSAGE(actual[i] == expected[i],
                "\nexpected: " << expected[i] << "\n"
                        << "actual: " << actual[i] << "\n");
    }
}