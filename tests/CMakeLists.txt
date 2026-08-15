add_subdirectory(autobahn)

# copy cert.pem public key to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/certs/cert.pem
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

# --------------------------------------
file(GLOB_RECURSE SOURCES_TESTS "./impl/*.cpp")
add_executable(unit_tests ${SOURCES_TESTS})

include(GoogleTest)
gtest_discover_tests(unit_tests)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gtest
    ZLIB::ZLIB
    OpenSSL::SSL
    OpenSSL::Crypto
    websocketclient
)

add_library(api_compile_tests OBJECT compile/api_instantiation.cpp)
target_link_libraries(api_compile_tests PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    websocketclient
)

add_subdirectory(close)
