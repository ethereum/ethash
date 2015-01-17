#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libdaggerhashimoto/num.h>
#include "internal.h"

int main(void){
  const num
    header = read_num("123"),
    seed = read_num("7");
  const int
    size = 78130000,
    //cache_size = size / 2,
    cache_size = 1,
    k = 1,
    trials = 1000;
  unsigned int hash_bit = 0;
  num * dag = malloc(sizeof(num)*size);
  //num * cache = malloc(sizeof(num)*cache_size);
  int nonce = 0;
  clock_t begin, end;
  double time_spent;
  parameters params = get_default_params();

  params.n = cache_size;
  params.cache_size = cache_size;
  params.k = k;
  //fprintf(stderr, "Making Cache\n");
  //produce_dag(cache, params, seed);
  //fprintf(stderr, "Cache Made\n");
  params.n = size;
  fprintf(stderr, "Making DAG\n");
  produce_dag(dag, params, seed);
  fprintf(stderr, "DAG Made\n");
  for (int accesses = 100; accesses < 300 ; accesses += 1) {
    params.accesses = accesses;
    begin = clock();
    for (int i = 0 ; i < trials ; i++) {
      hash_bit ^= num_to_uint(hashimoto(dag, params, header, nonce++)) & 1;
      //hash_bit ^= num_to_uint(light_hashimoto_cached(cache, params, header, nonce++)) & 1;
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%i\t%f\t%d\n", accesses, time_spent, hash_bit);
    fflush(stdout);
  }
  return EXIT_SUCCESS;
}
