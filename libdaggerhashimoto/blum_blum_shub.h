#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SAFE_PRIME 4294967087U
#define SAFE_PRIME_TOTIENT 4294967086U
#define SAFE_PRIME_TOTIENT_TOTIENT 2147483542
#define SAFE_PRIME2 4294965887U

uint32_t cube_mod_safe_prime1(const uint32_t x);
uint32_t cube_mod_safe_prime2(const uint32_t x);
uint32_t three_pow_mod_totient1(uint32_t p);
void init_power_table_mod_prime1(uint32_t table[32], const uint32_t n);
uint32_t quick_bbs(const uint32_t power_table[32], const uint64_t p);

#ifdef __cplusplus
}
#endif // __cplusplus
