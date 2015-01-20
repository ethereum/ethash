#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libethash/ethash.h>


int main(void) {
    ethash_params params;
    ethash_params_init(&params);
    params.full_size = 262147 * 4096;	// 1GBish;
    params.cache_size = 8209*4096;
    params.k = 2 * (params.full_size / params.cache_size);
    void* mem_buf = malloc(params.cache_size);

    ethash_cache cache;
    ethash_cache_init(&cache, mem_buf);

    printf("Building cache...\n");
    clock_t startTime = clock();
    ethash_compute_cache_data(&cache, &params);
    clock_t endTime = clock();
    printf("Time to build Cache: %lu msecs\n", (unsigned long) ((endTime - startTime) * 1000.0 / CLOCKS_PER_SEC));

    free(mem_buf);
    return EXIT_SUCCESS;
}