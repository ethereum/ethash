find_package( Boost COMPONENTS unit_test_framework REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )
include_directories(..)
link_directories ( ${Boost_LIBRARY_DIRS} )

add_executable (Test test.cpp)
target_link_libraries (Test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ethash)

enable_testing ()
add_test(NAME ethash COMMAND Test)
