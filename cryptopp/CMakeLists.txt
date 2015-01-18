set(LIBRARY cryptopp)

include_directories(../../cryptopp)

# todo, subset
file(GLOB HEADERS "../../cryptopp/*.h")
file(GLOB SOURCE "../../cryptopp/*.cpp")

add_library(${LIBRARY} ${HEADERS} ${SOURCE})
