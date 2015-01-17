set(CMAKE_BUILD_TYPE Release)
set(LIBRARY daggerhashimoto)

add_library(${LIBRARY} sha3.c daggerhashimoto.c bitfn.h compiler.h blum_blum_shub.c)
