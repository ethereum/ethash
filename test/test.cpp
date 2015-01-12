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

#include <gmp.h>

#include <libdaggerhashimoto/num.h>
#include <libdaggerhashimoto/daggerhashimoto.h>


#define BOOST_TEST_MODULE Daggerhashimoto
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <cstring>
#include <cstdlib>


BOOST_AUTO_TEST_CASE(num_type_has_desired_number_of_bits) {
    BOOST_CHECK_EQUAL(CHAR_BIT * sizeof(num), NUM_BITS);
}

BOOST_AUTO_TEST_CASE(num_from_string_and_back) {
    const char *in = "939079079942597099570608";
    num n = read_num(in);
    char out[155];
    write_num(out, n);

    BOOST_REQUIRE_MESSAGE(
            !strcmp(in, out),
            "Input String for read_num : " << in << "\n" <<
                    "Differs from output String from write_num : " << out << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_from_string_and_back_BIG) {
    const char *expected = "1340780792994259709957402499820584612747936582059239337772356144372176403007354697680187429816690342769003185818648605085375388281194656994643364900608";
    num n = read_num(expected);
    char actual[155];
    write_num(actual, n);

    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(double_num_from_string_and_back) {
    const char *in = "179769313486231590772930519078902473361797697894230657273430081157732675805500963132708477322407536021120113879871393357658789768814416622492847430639458866292343618749366170035854261348353022601149248808421156355915195314876239185014312577589513301062639254429970077114075674019839721108236314837055300759729";
    const double_num n = read_double_num(in);
    char out[309];
    write_double_num(out, n);

    BOOST_REQUIRE_MESSAGE(
            !strcmp(in, out),
            "Input String for read_num : " << in << "\n" <<
                    "Differs from output String from write_num : " << out << "\n"
    );
}

BOOST_AUTO_TEST_CASE(uint_to_num_and_back) {
    const unsigned int expected = 123456;
    const unsigned int actual = num_to_uint(uint_to_num(expected));
    BOOST_REQUIRE_MESSAGE(
            expected == actual,
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

/* TODO: test little endian-ness of num */
/* TODO: test mp_limb_t is 64 bits */

BOOST_AUTO_TEST_CASE(uint_to_double_num_and_back) {
    const unsigned int expected = 123456;
    const unsigned int actual = double_num_to_uint(uint_to_double_num(expected));
    BOOST_REQUIRE_MESSAGE(
            expected == actual,
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}


BOOST_AUTO_TEST_CASE(rshift) {
    const char *init = "3273390607896141870013189696827599152216642046043064789483291368096133796404674554883270092325904157150886684127560071009217256545885393053328527589376"; // 2**500
    const char *expected = "2977131414714805823690030317109266572712515013375254774912983855843898524112477893944078543723575564536883288499266264815757728270805630976"; // 2**460
    const int r = 40;
    char actual[155];

    BOOST_REQUIRE_MESSAGE (
            r < mp_bits_per_limb - 1,
            "Cannot shift right by " << r << "\n" <<
                    "Must be within 1 and " << mp_bits_per_limb - 1 << "\n"
    );

    write_num(actual, num_rshift(read_num(init), r));
    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(rshift_in_place) {
    const char *init = "136891479058588375991326027382088315966463695625337436471480190078368997177499076593800206155688941388250484440597994042813512732765695774566001"; // 3**300
    const char *expected = "130549887713039756766630198843086544004882522225701748343925657347077366998194767564583021312416974437952503624532693903745186550870605253"; // 3**300 >> 20
    int r = 20;
    num n = read_num(init);
    char actual[155];

    BOOST_REQUIRE_MESSAGE (
            1 < r && r < mp_bits_per_limb - 1,
            "Cannot shift right by " << r << "\n" <<
                    "Must be within 1 and " << mp_bits_per_limb - 1 << "\n"
    );

    num_rshift_in_place(&n, r);
    write_num(actual, n);

    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(lshift_in_place) {
    const char *init = "136891479058588375991326027382088315966463695625337436471480190078368997177499076593800206155688941388250484440597994042813512732765695774566001"; // 3**300
    const char *expected = "143541119545338364943480680488200638002850636104033827785518811791615849584393271738420644969907687405126139972784482201437221927272522212511319064576"; // 3**300 << 20
    int l = 20;
    num n = read_num(init);
    char actual[155];

    BOOST_REQUIRE_MESSAGE (
            1 < l && l < mp_bits_per_limb - 1,
            "Cannot shift left by " << l << "\n" <<
                    "Must be within 1 and " << mp_bits_per_limb - 1 << "\n"
    );

    num_lshift_in_place(&n, l);
    write_num(actual, n);

    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(lshift) {
    const char *init = "136891479058588375991326027382088315966463695625337436471480190078368997177499076593800206155688941388250484440597994042813512732765695774566001"; // 3**300
    const char *expected = "143541119545338364943480680488200638002850636104033827785518811791615849584393271738420644969907687405126139972784482201437221927272522212511319064576"; // 3**300 << 20
    int l = 20;
    num n = read_num(init);
    char actual[155];

    BOOST_REQUIRE_MESSAGE (
            1 < l && l < mp_bits_per_limb - 1,
            "Cannot shift left by " << l << "\n" <<
                    "Must be within 1 and " << mp_bits_per_limb - 1 << "\n"
    );

    write_num(actual, num_lshift(n, l));
    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_add_check) {
    const char *expected = "1212343670686395048563591569786948697866488587498701151742760761592185398613009240111341397627819625596206484768755372074328327143196291222809290107174859"; // 3**300 << 20
    num a = read_num("53942386653180348132335860181048318873003497138215325425249175919578619530592"),
            b = read_num("1212343670686395048563591569786948697866488587498701151742760761592185398612955297724688217279687289736025436449882368577190111817771042046889711487644267");
    char actual[155];
    write_num(actual, num_add(a, b));

    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(double_num_add_check) {
    const char *expected = "179769313486231590772930519078902473361797697894230657273430081157732675805500963132708477322407536021120113879871393357658789768814416622492847430639458866292343618749366170035854261348353022601149248808421156355915195314876239185014312577589513301062639254429970077114075674019839721108236314837055300759729";
    double_num a = read_double_num("25681330498033084396132931296986067623113956842032951039061440165390382257928709018958353903201076574445730554267341908236969966973488088927549632948494123756049088392766595719407751621193288943021321258345879479416456473553748455002044653941359043008948464918567153873439382002834245872605187833865042965675"),
            b = read_double_num("154087982988198506376797587781916405738683741052197706234368640992342293547572254113750123419206459446674383325604051449421819801840928533565297797690964742536294530356599574316446509727159733658127927550075276876498738841322490730012267923648154258053690789511402923240636292017005475235631127003190257794054");
    char actual[310];
    write_double_num(actual, double_num_add(a, b));

    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(add_in_place) {
    const char *expected = "1212343670686395048563591569786948697866488587498701151742760761592185398613009240111341397627819625596206484768755372074328327143196291222809290107174859"; // 3**300 << 20
    num a = read_num("53942386653180348132335860181048318873003497138215325425249175919578619530592"),
            b = read_num("1212343670686395048563591569786948697866488587498701151742760761592185398612955297724688217279687289736025436449882368577190111817771042046889711487644267");
    char actual[155];
    num_add_in_place(&a, b);
    write_num(actual, a);

    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_cmp_check) {
    const num a = read_num("999999123");
    const num b = uint_to_num(999999123);
    char a_[155], b_[155];
    write_num(a_, a);
    write_num(b_, b);
    BOOST_REQUIRE_MESSAGE(
            !num_cmp(a, b),
            "a : " << a_ << "\n" <<
                    "b : " << b_ << "\n" <<
                    "num_cmp(a,b) :" << num_cmp(a, b) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(double_num_cmp_check) {
    const double_num a = read_double_num("999999123");
    const double_num b = uint_to_double_num(999999123);
    char a_[155], b_[155];
    write_double_num(a_, a);
    write_double_num(b_, b);
    BOOST_REQUIRE_MESSAGE(
            !double_num_cmp(a, b),
            "a : " << a_ << "\n" <<
                    "b : " << b_ << "\n" <<
                    "double_num_cmp(a,b) :" << double_num_cmp(a, b) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_one_check) {
    const num expected = read_num("1");
    char expected_[155], actual_[155];
    write_num(expected_, expected);
    write_num(actual_, num_one);
    BOOST_REQUIRE_MESSAGE(
            !num_cmp(expected, num_one),
            "expected : " << expected_ << "\n" <<
                    "actual : " << actual_ << "\n" <<
                    "num_cmp(expected,actual) :" << num_cmp(expected, num_one) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(double_num_one_check) {
    const double_num expected = read_double_num("1");
    char expected_[309], actual_[309];
    write_double_num(expected_, expected);
    write_double_num(actual_, double_num_one);
    BOOST_REQUIRE_MESSAGE(
            !double_num_cmp(expected, double_num_one),
            "expected : " << expected_ << "\n" <<
                    "actual : " << actual_ << "\n" <<
                    "double_num_cmp(expected,actual) :" << double_num_cmp(expected, double_num_one) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_two_check) {
    const num expected = read_num("2");
    char expected_[155], actual_[155];
    write_num(expected_, expected);
    write_num(actual_, num_two);
    BOOST_REQUIRE_MESSAGE(
            !num_cmp(expected, num_two),
            "expected : " << expected_ << "\n" <<
                    "actual : " << actual_ << "\n" <<
                    "num_cmp(expected,actual) :" << num_cmp(expected, num_one) << "\n"
    );
}


BOOST_AUTO_TEST_CASE(double_num_is_zero_check) {
    BOOST_REQUIRE_MESSAGE(
            double_num_is_zero(uint_to_double_num(0)),
            "double_num_is_zero: " << double_num_is_zero(uint_to_double_num(0)) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(double_num_zero_is_zero) {
    BOOST_REQUIRE_MESSAGE(
            double_num_is_zero(double_num_zero),
            "double_num_is_zero: " << double_num_is_zero(double_num_zero) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_is_zero_check) {
    BOOST_REQUIRE_MESSAGE(
            num_is_zero(uint_to_num(0)),
            "num_is_zero: " << num_is_zero(uint_to_num(0)) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_zero_is_zero) {
    BOOST_REQUIRE_MESSAGE(
            num_is_zero(num_zero),
            "num_is_zero: " << num_is_zero(num_zero) << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_to_double_num_check) {
    const char *expected_ = "8706114721766558980485734853328017658561886823668819262184939983948513367304287722477862722471636197848708594394836109807760911049290488836351617405492450";
    double_num actual = num_to_double_num(read_num(expected_));
    char actual_[309];
    write_double_num(actual_, actual);
    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected_, actual_),
            "Expected: " << expected_ << "\n" <<
                    "Actual: " << actual_ << "\n"
    );
}

BOOST_AUTO_TEST_CASE(double_num_to_num_check) {
    const char *expected_ = "6706114721766558980485734853328017658561886823668819262184939983948513367304287722477862722471636197848708594394836109807760911049290488836351617405492450";
    num actual = double_num_to_num(read_double_num(expected_));
    char actual_[155];
    write_num(actual_, actual);
    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected_, actual_),
            "Expected: " << expected_ << "\n" <<
                    "Actual: " << actual_ << "\n"
    );
}

BOOST_AUTO_TEST_CASE(xor_in_place_check) {
    const char *expected = "9706114721766558980485734853328017658561886823668819262184939983948513367304287722477862722471636197848708594394836109807760911049290488836351617405492450";
    num a = read_num("3569861326704761654270430977326368488493176709259110883559590691573033693030486005617528091939430470747927935871833691598296836515910582609523621536923953");
    const num b = read_num("13275975848630389716704730759507547178881927740570887943439562718913314263594234954947771687452288302039086384165241687046787462673491991882670234960928211");
    num_xor_in_place(&a, b);
    char actual[155];
    write_num(actual, a);
    BOOST_REQUIRE_MESSAGE(
            !strcmp(expected, actual),
            "Expected : " << expected << "\n" <<
                    "Actual : " << actual << "\n"
    );
}

BOOST_AUTO_TEST_CASE(num_multiply_check) {
    const char *expected_ = "36360291795869936842385267079543319118023385026001623040346035832580600191583895484198508262979388783308179702534403855752855931517013066142992430916562025780021771247847643450125342836565813209972590371590152578728008385990139795377610001"; // 3**500
    const double_num expected = read_double_num(expected_);
    const num a = read_num("265613988875874769338781322035779626829233452653394495974574961739092490901302182994384699044001"); // 3**200
    const num b = read_num("136891479058588375991326027382088315966463695625337436471480190078368997177499076593800206155688941388250484440597994042813512732765695774566001"); // 3**300
    const double_num actual = num_multiply(a, b);
    char actual_[309];
    write_double_num(actual_, actual);
    BOOST_REQUIRE_MESSAGE(
            !double_num_cmp(actual, expected),
            "expected: " << expected_ << "\n" <<
                    "actual: " << actual_ << "\n");
}

BOOST_AUTO_TEST_CASE(num_multiply_in_place_check) {
    const char *expected_ = "36360291795869936842385267079543319118023385026001623040346035832580600191583895484198508262979388783308179702534403855752855931517013066142992430916562025780021771247847643450125342836565813209972590371590152578728008385990139795377610001"; // 3**500
    const double_num expected = read_double_num(expected_);
    const num a = read_num("265613988875874769338781322035779626829233452653394495974574961739092490901302182994384699044001"); // 3**200
    const num b = read_num("136891479058588375991326027382088315966463695625337436471480190078368997177499076593800206155688941388250484440597994042813512732765695774566001"); // 3**300
    double_num actual;
    num_multiply_to_addr(&actual, a, b);
    char actual_[309];
    write_double_num(actual_, actual);
    BOOST_REQUIRE_MESSAGE(
            !double_num_cmp(actual, expected),
            "expected: " << expected_ << "\n" <<
                    "actual: " << actual_ << "\n");
}

BOOST_AUTO_TEST_CASE(double_mod_num_check) {
    const char *expected_ = "2029808739295579349506219913235020474714774888322204745831858489634907480888360098067545807456498774147843755047498994386381227839833409"; // 3**400 % 5**50
    const num expected = read_num(expected_);
    const double_num a = read_double_num("70550791086553325712464271575934796216507949612787315762871223209262085551582934156579298529447134158154952334825355911866929793071824566694145084454535257027960285323760313192443283334088001"); // 3**400
    const num b = read_num("2907354897182427562197295231552018137414565442749272241125960796722557152453591693304764202855054262243050086425064711734138406514458624"); // 2**450
    const num actual = double_num_mod_num(a, b);
    char actual_[155];
    write_num(actual_, actual);
    BOOST_REQUIRE_MESSAGE(
            !num_cmp(actual, expected),
            "expected: " << expected_ << "\n" <<
                    "actual: " << actual_ << "\n"
    );
}

/********************************************/
// daggerhashimoto.h checks

//BOOST_AUTO_TEST_CASE(encode_num_as_string)
//{
//  const char * expected =
//    "-_-_-_-_-_-_-"
//    "a man in the depths of an ether binge."
//    "-_-_-_-_-_-_-";
//  const num n = read_num("2376313254082295953483669800170309493694116695737382715009547212327485292275330050638210649550095629709409670607075760221038595165073824153062320546602797");
//  enc_num actual = encode_num(n);
//  BOOST_REQUIRE_MESSAGE(
//    strlen(expected) == ENCODED_NUM_BYTES,
//    "Expected ENCODED_NUM_BYTES " << ENCODED_NUM_BYTES <<
//    " to be " << strlen(expected) << "\n" <<
//    "(the length of the string \"" << expected << "\")"
//    );
//  BOOST_REQUIRE_MESSAGE(
//    !std::memcmp(expected, actual.char_array, ENCODED_NUM_BYTES),
//    "Expected: \"" << expected << "\"\n" <<
//    "Actual: \"" << actual.char_array << "\"\n" <<
//    "ENCODED_NUM_BYTES : " << ENCODED_NUM_BYTES
//    );
//}

//BOOST_AUTO_TEST_CASE(sha3_check)
//{
//  const char * expected = "103588989910066412099008601990527779123911989727766679804758218239207085010305";
//  const num result = num_sha3(read_num("32483691262734967725664914442765702524327428635899764086166644844913352168645"));
//  char actual[155];
//  write_num(actual, result);
//  BOOST_REQUIRE_MESSAGE(
//    !strcmp(expected, actual),
//    "Expected : " << expected << "\n" <<
//    "Actual : " << actual << "\n"
//    );
//}
//
//BOOST_AUTO_TEST_CASE(double_mod_default_param_P)
//{
//  const char * expected_ = "8811608540281055256345643362681227616461045216252211008426951355398271759222495465685760399125663211250914198651576527278069568242872816674747523441537135"; // 3**400 % P
//  const num expected = read_num(expected_);
//  const double_num a = read_double_num("70550791086553325712464271575934796216507949612787315762871223209262085551582934156579298529447134158154952334825355911866929793071824566694145084454535257027960285323760313192443283334088001"); // 3**400
//  const num P = get_default_params().P;
//  const num actual = double_num_mod_num(a, P);
//  char actual_[155];
//  write_num(actual_, actual);
//  BOOST_REQUIRE_MESSAGE(
//    !num_cmp(actual, expected),
//    "expected: " << expected_ << "\n" <<
//    "actual: " << actual_ << "\n"
//    );
//}
//
//BOOST_AUTO_TEST_CASE(num_mod_uint_check)
//{
//  const num a = read_num("62230152778611417071440640537801242405902521687211671331011166147896988340353834411839448231257136169569665895551224821247160434722900390625");
//  const int expected = 5569099,
//    actual = num_mod_uint(a, 9999999);
//  BOOST_REQUIRE_MESSAGE(
//    actual == expected,
//    "expected: " << expected << "\n" <<
//    "actual: " << actual << "\n"
//    );
//}
//
//BOOST_AUTO_TEST_CASE(num_is_odd_check)
//{
//  BOOST_REQUIRE_MESSAGE(
//    !num_is_odd(uint_to_num(0)),
//    "num_is_odd(uint_to_num(0)): " << num_is_odd(uint_to_num(0)) << "\n"
//    );
//  BOOST_REQUIRE_MESSAGE(
//    num_is_odd(uint_to_num(1)),
//    "num_is_odd(uint_to_num(1)): " << num_is_odd(uint_to_num(1)) << "\n"
//    );
//}
//
//BOOST_AUTO_TEST_CASE(num_pow_mod_check){
//  const char * expected_ = "55515194935817844097553491101463823935464577853304007830633332799810844581067302449379020413102163449283115591982882117345932606270589843751"; // pow(3**300, 1000000, 5**200)
//  const num
//    m = read_num("62230152778611417071440640537801242405902521687211671331011166147896988340353834411839448231257136169569665895551224821247160434722900390625"), // 5**200
//    a = read_num("136891479058588375991326027382088315966463695625337436471480190078368997177499076593800206155688941388250484440597994042813512732765695774566001"), // 3**300
//    expected = read_num(expected_),
//    actual = num_pow_mod(a,1000000,m);
//  char actual_[155];
//  write_num(actual_, actual);
//
//  BOOST_REQUIRE_MESSAGE(
//    !num_cmp(expected,actual),
//    "expected: " << expected_ << "\n" <<
//    "actual: " << actual_ << "\n"
//    );
//}
//
//BOOST_AUTO_TEST_CASE(num_mult_mod_check){
//  const num
//    a = read_num("30834610858062074296448642597405756808220843024709445075135965107038834920948"),
//    P = read_num("13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006045979");
//  {
//    const char * expected = "950773226768119569714559904964937336983928087723880850490000708362484574442340375342810492566468892628466140782865499580645227699565570873957037409218704";
//    char actual[155];
//    write_num(actual, num_mult_mod(a,a,P));
//    BOOST_REQUIRE_MESSAGE(
//      !strcmp(expected, actual),
//      "expected: " << expected << "\n" <<
//      "actual: " << actual << "\n"
//      );
//  }
//  {
//    const char * expected = "12946592222289761800348450503220384469365214983009637212441024283805102330842886752016150482813558946845758275119449160735119855410668970579005185818698054";
//    char actual[155];
//    write_num(actual, num_mult_mod(num_mult_mod(a,a,P), a, P));
//    BOOST_REQUIRE_MESSAGE(
//      !strcmp(expected, actual),
//      "expected: " << expected << "\n" <<
//      "actual: " << actual << "\n"
//      );
//  }
//}
//
//BOOST_AUTO_TEST_CASE(produce_dag_check){
//
//  const num seed = uint_to_num(999998888);
//  char actual_[155], expected_[155];
//  parameters params = get_default_params();
//  const int size = 5;
//  num
//    actual[size],
//    expected[size] = {
//    read_num("12946592222289761800348450503220384469365214983009637212441024283805102330842886752016150482813558946845758275119449160735119855410668970579005185818698054"),
//    read_num("9430959799509364199938662379808489869328324369163942920342372938348060180097135426429590093226718001816186038499249384755013798094542981363448277939871326"),
//    read_num("4301974854406447892343667082403677551908600916016924633151292713424651911527918363072571014008599478867821054040948406800439759977246463008561179084152473"),
//    read_num("4599698502906874114955027459370086974435665624602048196342493677810448976006006812496698306930007750793298338614942082946718772173829843647556459973918882"),
//    read_num("6293417495270500823502726444679175878463226905969000272807059440329134133791874932739923780691426852726556618914123449607646693495875285018593048211681023")
//  };
//  params.n = size;
//  produce_dag(actual, params, seed);
//  for(int i = 0 ; i < size ; ++i) {
//    write_num(expected_, expected[i]);
//    write_num(actual_, actual[i]);
//    BOOST_REQUIRE_MESSAGE(
//      !num_cmp(expected[i],actual[i]),
//      "i: " << i << "\n" <<
//      "expected: " << expected_ << "\n" <<
//      "actual: " << actual_ << "\n"
//      );
//  }
//}
//
//BOOST_AUTO_TEST_CASE(quick_calc_check) {
//  parameters params = get_default_params();
//  const int size = 100;
//  num expected[size];
//  const num seed = read_num("123123123123123123123123123123123123");
//  char expected_[155], actual_[155];
//  params.n = size;
//  params.cache_size = size / 10;
//  produce_dag(expected, params, seed);
//  for(int i = 0 ; i < size ; ++i) {
//    write_num(expected_, expected[i]);
//    write_num(actual_, quick_calc(params, seed, i));
//    BOOST_REQUIRE_MESSAGE(
//      ! strcmp(expected_, actual_),
//      "i: " << i << "\n"
//      << "expected: " << expected_ << "\n"
//      << "actual: " << actual_ << "\n"
//      );
//  }
//}

//BOOST_AUTO_TEST_CASE(hashimoto_check) {
//  parameters params = get_default_params();
//  const int size = 100;
//  num dag[size];
//  const num
//    seed = read_num("7"),
//    header = read_num("123"),
//    nonce = read_num("800");
//  char expected[] = "97418297196838358458655356064520668367771298950637159168215218954926213481962",
//    actual[155];
//
//  params.n = size;
//  produce_dag(dag, params, seed);
//  write_num(actual, hashimoto(dag, params, header, nonce));
//
//  BOOST_REQUIRE_MESSAGE(
//    ! strcmp(expected, actual),
//    "expected: " << expected << "\n"
//    << "actual: " << actual << "\n"
//    );
//}
//
//BOOST_AUTO_TEST_CASE(quick_hashimoto_check) {
//  parameters params = get_default_params();
//  const int size = 100;
//  num dag[size];
//  const num
//    seed = read_num("7"),
//    header = read_num("123"),
//    nonce = read_num("800");
//  char expected[155],
//    actual[155];
//
//  params.n = size;
//  params.cache_size = 10;
//  produce_dag(dag, params, seed);
//  write_num(expected, hashimoto(dag, params, header, nonce));
//  write_num(actual, quick_hashimoto(seed, params, header, nonce));
//
//  BOOST_REQUIRE_MESSAGE(
//    ! strcmp(expected, actual),
//    "expected: " << expected << "\n"
//    << "actual: " << actual << "\n"
//    );
// }
