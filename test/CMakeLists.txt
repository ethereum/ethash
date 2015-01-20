find_package( Boost COMPONENTS unit_test_framework REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )
include_directories(..)

link_directories ( ${Boost_LIBRARY_DIRS} )
file(GLOB HEADERS "*.h")
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
find_package (CryptoPP)

if (CRYPTOPP_FOUND)
	add_definitions(-DWITH_CRYPTOPP)
endif()

add_executable (Test test.cpp ${HEADERS})
target_link_libraries (Test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${ETHHASH_LIBS})

if (CRYPTOPP_FOUND)
	TARGET_LINK_LIBRARIES(Test ${CRYPTOPP_LIBRARIES})
endif()

enable_testing ()
add_test(NAME ethash COMMAND Test)