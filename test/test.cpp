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


std::string hashToHex(const unsigned char str[HASH_CHARS]) {
    std::ostringstream ret;

    for (int i = 0; i < HASH_CHARS; ++i)
        ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int) str[i];

    return ret.str();
}

BOOST_AUTO_TEST_CASE(num_from_string_and_back) {
    unsigned char result[HASH_CHARS];
    //sha3_1(result, defaults.diff);

    const unsigned char expected[HASH_CHARS] = {
            43, 93, 223, 111, 77, 33, 194, 61, 226, 22, 244, 77, 94, 75, 220,
            104, 224, 68, 183, 24, 151, 131, 126, 167, 76, 131, 144, 139, 231, 3, 124, 215};

    BOOST_REQUIRE_MESSAGE(
            !memcmp(result, expected, HASH_CHARS),
            "\nExpected : " + hashToHex(expected) + "\n" +
                    "Actual : " + hashToHex(result) + "\n"
    );
}

