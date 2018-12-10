#include <iomanip>
#include <libethash/fnv.h>
#include <libethash/ethash.h>
#include <libethash/internal.h>
#include <libethash/io.h>

#ifdef WITH_CRYPTOPP

#include <libethash/sha3_cryptopp.h>

#else
#include <libethash/sha3.h>
#endif // WITH_CRYPTOPP

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using byte = uint8_t;
using bytes = std::vector<byte>;
namespace fs = boost::filesystem;

// Just an alloca "wrapper" to silence uint64_t to size_t conversion warnings in windows
// consider replacing alloca calls with something better though!
#define our_alloca(param__) alloca((size_t)(param__))

// some functions taken from eth::dev for convenience.
static std::string bytesToHexString(const uint8_t *str, const uint64_t s)
{
	std::ostringstream ret;

	for (size_t i = 0; i < s; ++i)
		ret << std::hex << std::setfill('0') << std::setw(2) << std::nouppercase << (int) str[i];

	return ret.str();
}

static std::string blockhashToHexString(ethash_h256_t* _hash)
{
	return bytesToHexString((uint8_t*)_hash, 32);
}

static int fromHex(char _i)
{
	if (_i >= '0' && _i <= '9')
		return _i - '0';
	if (_i >= 'a' && _i <= 'f')
		return _i - 'a' + 10;
	if (_i >= 'A' && _i <= 'F')
		return _i - 'A' + 10;

	BOOST_REQUIRE_MESSAGE(false, "should never get here");
	return -1;
}

static bytes hexStringToBytes(std::string const& _s)
{
	unsigned s = (_s[0] == '0' && _s[1] == 'x') ? 2 : 0;
	std::vector<uint8_t> ret;
	ret.reserve((_s.size() - s + 1) / 2);

	if (_s.size() % 2)
		try
		{
			ret.push_back(fromHex(_s[s++]));
		}
		catch (...)
		{
			ret.push_back(0);
		}
	for (unsigned i = s; i < _s.size(); i += 2)
		try
		{
			ret.push_back((byte)(fromHex(_s[i]) * 16 + fromHex(_s[i + 1])));
		}
		catch (...){
			ret.push_back(0);
		}
	return ret;
}

static ethash_h256_t stringToBlockhash(std::string const& _s)
{
	ethash_h256_t ret;
	bytes b = hexStringToBytes(_s);
	memcpy(&ret, b.data(), b.size());
	return ret;
}

/* ProgPoW */

static void ethash_keccakf800(uint32_t state[25])
{
    for (int i = 0; i < 22; ++i)
        keccak_f800_round(state, i);
}

BOOST_AUTO_TEST_CASE(test_progpow_math)
{
	typedef struct {
		uint32_t a;
		uint32_t b;
		uint32_t exp;
	} mytest;

	mytest tests[] = {
		{20, 22, 42},
		{70000, 80000, 1305032704},
		{70000, 80000, 1},
		{1, 2, 1},
		{3, 10000, 196608},
		{3, 0, 3},
		{3, 6, 2},
		{3, 6, 7},
		{3, 6, 5},
		{0, 0xffffffff, 32},
		{3 << 13, 1 << 5, 3},
		{22, 20, 42},
		{80000, 70000, 1305032704},
		{80000, 70000, 1},
		{2, 1, 1},
		{10000, 3, 80000},
		{0, 3, 0},
		{6, 3, 2},
		{6, 3, 7},
		{6, 3, 5},
		{0, 0xffffffff, 32},
		{3 << 13, 1 << 5, 3},
	};

	for (int i = 0; i < sizeof(tests) / sizeof(mytest); i++) {
		uint32_t res = progpowMath(tests[i].a, tests[i].b, (uint32_t)i);
		BOOST_REQUIRE_EQUAL(res, tests[i].exp);
	}
}

BOOST_AUTO_TEST_CASE(test_progpow_merge)
{
	typedef struct {
		uint32_t a;
		uint32_t b;
		uint32_t exp;
	} mytest;
	mytest tests[] = {
		{1000000, 101, 33000101},
		{2000000, 102, 66003366},
		{3000000, 103, 2999975},
		{4000000, 104, 4000104},
		{1000000, 0, 33000000},
		{2000000, 0, 66000000},
		{3000000, 0, 3000000},
		{4000000, 0, 4000000},
	};
	for (int i = 0; i < sizeof(tests) / sizeof(mytest); i++) {
		uint32_t res = tests[i].a;
		merge(&res, tests[i].b, (uint32_t)i);
		BOOST_REQUIRE_EQUAL(res, tests[i].exp);
	}
}

BOOST_AUTO_TEST_CASE(test_progpow_keccak)
{
	// Test vectors from
	// https://github.com/XKCP/XKCP/blob/master/tests/TestVectors/KeccakF-800-IntermediateValues.txt.
	uint32_t state[25] = {};
	const uint32_t expected_state_0[] = {0xE531D45D, 0xF404C6FB, 0x23A0BF99, 0xF1F8452F, 0x51FFD042,
		0xE539F578, 0xF00B80A7, 0xAF973664, 0xBF5AF34C, 0x227A2424, 0x88172715, 0x9F685884,
		0xB15CD054, 0x1BF4FC0E, 0x6166FA91, 0x1A9E599A, 0xA3970A1F, 0xAB659687, 0xAFAB8D68,
		0xE74B1015, 0x34001A98, 0x4119EFF3, 0x930A0E76, 0x87B28070, 0x11EFE996};
	ethash_keccakf800(state);
	for (size_t i = 0; i < 25; ++i)
		BOOST_REQUIRE_EQUAL(state[i], expected_state_0[i]);
	const uint32_t expected_state_1[] = {0x75BF2D0D, 0x9B610E89, 0xC826AF40, 0x64CD84AB, 0xF905BDD6,
		0xBC832835, 0x5F8001B9, 0x15662CCE, 0x8E38C95E, 0x701FE543, 0x1B544380, 0x89ACDEFF,
		0x51EDB5DE, 0x0E9702D9, 0x6C19AA16, 0xA2913EEE, 0x60754E9A, 0x9819063C, 0xF4709254,
		0xD09F9084, 0x772DA259, 0x1DB35DF7, 0x5AA60162, 0x358825D5, 0xB3783BAB};
	ethash_keccakf800(state);
	for (size_t i = 0; i < 25; ++i)
		BOOST_REQUIRE_EQUAL(state[i], expected_state_1[i]);
}

BOOST_AUTO_TEST_CASE(test_progpow_block0_verification) {
	// epoch 0
	ethash_light_t light = ethash_light_new(1045);
	ethash_h256_t seedhash = stringToBlockhash("5fc898f16035bf5ac9c6d9077ae1e3d5fc1ecc3c9fd5bee8bb00e810fdacbaa0");
	BOOST_ASSERT(light);
	ethash_return_value_t ret = progpow_light_compute(
		light,
		seedhash,
		0x50377003e5d830caU,
		1045
	);
	//ethash_h256_t difficulty = ethash_h256_static_init(0x25, 0xa6, 0x1e);
	//BOOST_REQUIRE(ethash_check_difficulty(&ret.result, &difficulty));
	ethash_light_delete(light);
}

BOOST_AUTO_TEST_CASE(test_progpow_keccak_f800) {
	ethash_h256_t seedhash;
	ethash_h256_t headerhash = stringToBlockhash("0000000000000000000000000000000000000000000000000000000000000000");

	{
		const std::string
			seedexp = "5dd431e5fbc604f499bfa0232f45f8f142d0ff5178f539e5a7800bf0643697af";
		const std::string header_string = blockhashToHexString(&headerhash);
		BOOST_REQUIRE_MESSAGE(true,
				"\nheader: " << header_string.c_str() << "\n");
		hash32_t result;
		for (int i = 0; i < 8; i++)
			result.uint32s[i] = 0;

		hash32_t header;
		memcpy((void *)&header, (void *)&headerhash, sizeof(headerhash));
		uint64_t nonce = 0x0;
		// keccak(header..nonce)
		hash32_t seed_256 = keccak_f800_progpow(header, nonce, result);
		uint64_t seed = (uint64_t)ethash_swap_u32(seed_256.uint32s[0]) << 32 | ethash_swap_u32(seed_256.uint32s[1]);
		uint64_t exp = 0x5dd431e5fbc604f4U;

		BOOST_REQUIRE_MESSAGE(seed == exp,
				"\nseed: " << seed << "exp: " << exp << "\n");
		ethash_h256_t out;
		memcpy((void *)&out, (void *)&seed_256, sizeof(result));
		const std::string out_string = blockhashToHexString(&out);
		BOOST_REQUIRE_MESSAGE(out_string == seedexp,
				"\nresult: " << out_string.c_str() << "\n");
	}
}

BOOST_AUTO_TEST_CASE(test_progpow_full_client_checks) {
	uint64_t full_size = ethash_get_datasize(0);
	uint64_t cache_size = ethash_get_cachesize(0);
	ethash_h256_t difficulty;
	ethash_return_value_t light_out;
	ethash_return_value_t full_out;
	ethash_h256_t hash = stringToBlockhash("0000000000000000000000000000000000000000000000000000000000000000");
	ethash_h256_t seed = stringToBlockhash("0000000000000000000000000000000000000000000000000000000000000000");

	// Set the difficulty
	ethash_h256_set(&difficulty, 0, 197);
	ethash_h256_set(&difficulty, 1, 90);
	for (int i = 2; i < 32; i++)
		ethash_h256_set(&difficulty, i, 255);

	ethash_light_t light = ethash_light_new_internal(cache_size, &seed);
	ethash_full_t full = ethash_full_new_internal(
		"./test_ethash_directory/",
		seed,
		full_size,
		light,
		NULL
	);
	{
		uint64_t nonce = 0x0;
		full_out = progpow_full_compute(full, hash, nonce, 0);
		BOOST_REQUIRE(full_out.success);

		const std::string
			exphead = "7ea12cfc33f64616ab7dbbddf3362ee7dd3e1e20d60d860a85c51d6559c912c4",
			expmix = "a09ffaa0f2b5d47a98c2d4fbc0e90936710dd2b2a220fce04e8d55a6c6a093d6";
		const std::string seed_string = blockhashToHexString(&seed);
		const std::string hash_string = blockhashToHexString(&hash);

		const std::string full_mix_hash_string = blockhashToHexString(&full_out.mix_hash);
		BOOST_REQUIRE_MESSAGE(full_mix_hash_string == expmix,
				"\nfull mix hash: " << full_mix_hash_string.c_str() << "\n");
		const std::string full_result_string = blockhashToHexString(&full_out.result);
		BOOST_REQUIRE_MESSAGE(full_result_string == exphead,
				"\nfull result: " << full_result_string.c_str() << "\n");
	}

	ethash_light_delete(light);
	ethash_full_delete(full);
	//fs::remove_all("./test_ethash_directory/");
}

BOOST_AUTO_TEST_CASE(test_progpow_light_client_checks) {
	uint64_t full_size = ethash_get_datasize(0);
	uint64_t cache_size = ethash_get_cachesize(0);
	ethash_return_value_t light_out;
	ethash_h256_t hash = stringToBlockhash("0000000000000000000000000000000000000000000000000000000000000000");
	ethash_h256_t seed = stringToBlockhash("0000000000000000000000000000000000000000000000000000000000000000");
	ethash_light_t light = ethash_light_new_internal(cache_size, &seed);
	{
		uint64_t nonce = 0x0;
		const std::string
			exphead = "63155f732f2bf556967f906155b510c917e48e99685ead76ea83f4eca03ab12b",
			expmix = "faeb1be51075b03a4ff44b335067951ead07a3b078539ace76fd56fc410557a3";
		const std::string hash_string = blockhashToHexString(&hash);

		light_out = progpow_light_compute_internal(light, full_size, hash, nonce, 0);
		BOOST_REQUIRE(light_out.success);

		const std::string light_result_string = blockhashToHexString(&light_out.result);
		BOOST_REQUIRE_MESSAGE(exphead == light_result_string,
				"\nlight result: " << light_result_string.c_str() << "\n"
						<< "exp result: " << exphead.c_str() << "\n");
		const std::string light_mix_hash_string = blockhashToHexString(&light_out.mix_hash);
		BOOST_REQUIRE_MESSAGE(expmix == light_mix_hash_string,
				"\nlight mix hash: " << light_mix_hash_string.c_str() << "\n"
						<< "exp mix hash: " << expmix.c_str() << "\n");
	}

	ethash_light_delete(light);
}

/// Defines a test case for ProgPoW hash() function. (from chfast/ethash/test/unittests/progpow_test_vectors.hpp)
struct progpow_hash_test_case
{
	int block_number;
	const char* header_hash_hex;
	const char* nonce_hex;
	const char* mix_hash_hex;
	const char* final_hash_hex;
};

progpow_hash_test_case progpow_hash_test_cases[] = {
	{0, "0000000000000000000000000000000000000000000000000000000000000000", "0000000000000000",
		"faeb1be51075b03a4ff44b335067951ead07a3b078539ace76fd56fc410557a3",
		"63155f732f2bf556967f906155b510c917e48e99685ead76ea83f4eca03ab12b"},
	{49, "63155f732f2bf556967f906155b510c917e48e99685ead76ea83f4eca03ab12b", "0000000006ff2c47",
		"c789c1180f890ec555ff42042913465481e8e6bc512cb981e1c1108dc3f2227d",
		"9e7248f20914913a73d80a70174c331b1d34f260535ac3631d770e656b5dd922"},
	{50, "9e7248f20914913a73d80a70174c331b1d34f260535ac3631d770e656b5dd922", "00000000076e482e",
		"c7340542c2a06b3a7dc7222635f7cd402abf8b528ae971ddac6bbe2b0c7cb518",
		"de37e1824c86d35d154cf65a88de6d9286aec4f7f10c3fc9f0fa1bcc2687188d"},
	{99, "de37e1824c86d35d154cf65a88de6d9286aec4f7f10c3fc9f0fa1bcc2687188d", "000000003917afab",
		"f5e60b2c5bfddd136167a30cbc3c8dbdbd15a512257dee7964e0bc6daa9f8ba7",
		"ac7b55e801511b77e11d52e9599206101550144525b5679f2dab19386f23dcce"},
	{29950, "ac7b55e801511b77e11d52e9599206101550144525b5679f2dab19386f23dcce", "005d409dbc23a62a",
		"07393d15805eb08ee6fc6cb3ad4ad1010533bd0ff92d6006850246829f18fd6e",
		"e43d7e0bdc8a4a3f6e291a5ed790b9fa1a0948a2b9e33c844888690847de19f5"},
	{29999, "e43d7e0bdc8a4a3f6e291a5ed790b9fa1a0948a2b9e33c844888690847de19f5", "005db5fa4c2a3d03",
		"7551bddf977491da2f6cfc1679299544b23483e8f8ee0931c4c16a796558a0b8",
		"d34519f72c97cae8892c277776259db3320820cb5279a299d0ef1e155e5c6454"},
	{30000, "d34519f72c97cae8892c277776259db3320820cb5279a299d0ef1e155e5c6454", "005db8607994ff30",
		"f1c2c7c32266af9635462e6ce1c98ebe4e7e3ecab7a38aaabfbf2e731e0fbff4",
		"8b6ce5da0b06d18db7bd8492d9e5717f8b53e7e098d9fef7886d58a6e913ef64"},
	{30049, "8b6ce5da0b06d18db7bd8492d9e5717f8b53e7e098d9fef7886d58a6e913ef64", "005e2e215a8ca2e7",
		"57fe6a9fbf920b4e91deeb66cb0efa971e08229d1a160330e08da54af0689add",
		"c2c46173481b9ced61123d2e293b42ede5a1b323210eb2a684df0874ffe09047"},
	{30050, "c2c46173481b9ced61123d2e293b42ede5a1b323210eb2a684df0874ffe09047", "005e30899481055e",
		"ba30c61cc5a2c74a5ecaf505965140a08f24a296d687e78720f0b48baf712f2d",
		"ea42197eb2ba79c63cb5e655b8b1f612c5f08aae1a49ff236795a3516d87bc71"},
	{30099, "ea42197eb2ba79c63cb5e655b8b1f612c5f08aae1a49ff236795a3516d87bc71", "005ea6aef136f88b",
		"cfd5e46048cd133d40f261fe8704e51d3f497fc14203ac6a9ef6a0841780b1cd",
		"49e15ba4bf501ce8fe8876101c808e24c69a859be15de554bf85dbc095491bd6"},
	{59950, "49e15ba4bf501ce8fe8876101c808e24c69a859be15de554bf85dbc095491bd6", "02ebe0503bd7b1da",
		"21511fbaa31fb9f5fc4998a754e97b3083a866f4de86fa7500a633346f56d773",
		"f5c50ba5c0d6210ddb16250ec3efda178de857b2b1703d8d5403bd0f848e19cf"},
	{59999, "f5c50ba5c0d6210ddb16250ec3efda178de857b2b1703d8d5403bd0f848e19cf", "02edb6275bd221e3",
		"653eda37d337e39d311d22be9bbd3458d3abee4e643bee4a7280a6d08106ef98",
		"341562d10d4afb706ec2c8d5537cb0c810de02b4ebb0a0eea5ae335af6fb2e88"},
};

BOOST_AUTO_TEST_CASE(test_progpow_test_cases) {
	ethash_light_t light;
	uint32_t epoch = -1;
	for (int i = 0; i < sizeof(progpow_hash_test_cases) / sizeof(progpow_hash_test_case); i++)
	{
		progpow_hash_test_case *t;
		t = &progpow_hash_test_cases[i];
		const auto epoch_number = t->block_number / ETHASH_EPOCH_LENGTH;
		if (!light || epoch != epoch_number)
			light = ethash_light_new(t->block_number);
		epoch = epoch_number;
		ethash_h256_t hash = stringToBlockhash(t->header_hash_hex);
		uint64_t nonce = strtoul(t->nonce_hex, NULL, 16);
		ethash_return_value_t light_out = progpow_light_compute(light, hash, nonce, t->block_number);
		BOOST_REQUIRE_EQUAL(blockhashToHexString(&light_out.result), t->final_hash_hex);
		BOOST_REQUIRE_EQUAL(blockhashToHexString(&light_out.mix_hash), t->mix_hash_hex);
		printf("next...\n");
	}
	ethash_light_delete(light);
}
