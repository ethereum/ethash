set(LIBRARY ethash)

if (WANT_CRYPTOPP)
	set(SHA3_SUFFIX "_cryptopp")
	set(SHA3_EXT "pp")
endif()

add_library(${LIBRARY}
	util.c
	util.h
	ethash.c
	ethash.h
	blum_blum_shub.c
	blum_blum_shub.h
	bitfn.h
	compiler.h
	sha3${SHA3_SUFFIX}.h
	sha3${SHA3_SUFFIX}.c${SHA3_EXT}
	)
