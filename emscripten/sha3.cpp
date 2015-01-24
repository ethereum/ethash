#include <emscripten/bind.h>
#include <libethash/sha3.h>

using namespace emscripten;

std::string SHA256(std::string input) {
    uint8_t out[32];
    sha3_256(out, 32, (const uint8_t *) input.c_str(), input.length());
    return std::string((const char *) out, 32);
}

std::string SHA512(std::string input) {
    uint8_t out[64];
    sha3_512(out, 64, (const uint8_t *) input.c_str(), input.length());
    return std::string((const char *) out, 64);
}

EMSCRIPTEN_BINDINGS(SHA256) {
        function("SHA256", &SHA256);
        function("SHA512", &SHA512);
}