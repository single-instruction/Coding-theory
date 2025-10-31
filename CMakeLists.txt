cmake_minimum_required(VERSION 3.10)
project(codectk VERSION 1.0.0 LANGUAGES C)

# Compiler flags: strict warnings and optimizations
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wconversion -fPIC")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(CODECTK_SOURCES
  src/registry.c
  src/bitio.c
  src/gf2.c
  src/gf2m.c
  src/poly.c
  src/hamming.c
  src/huffman.c
  src/bch.c
  src/goppa.c
)

# Static library
add_library(codectk_static STATIC ${CODECTK_SOURCES})
set_target_properties(codectk_static PROPERTIES OUTPUT_NAME codectk)

# Shared library
add_library(codectk_shared SHARED ${CODECTK_SOURCES})
set_target_properties(codectk_shared PROPERTIES OUTPUT_NAME codectk)

# Install targets
install(TARGETS codectk_static codectk_shared
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include/codectk)

# Test suite
enable_testing()

# Test executable sources
set(TEST_SOURCES
  tests/test_main.c
  tests/test_bitio.c
  tests/test_gf2.c
  tests/test_gf2m.c
  tests/test_poly.c
  tests/test_hamming.c
  tests/test_huffman.c
  tests/test_bch.c
)

add_executable(test_codectk ${TEST_SOURCES})
target_link_libraries(test_codectk codectk_static m)
add_test(NAME codectk_tests COMMAND test_codectk)

# Example tools
add_executable(pipe_tool tools/pipe.c)
target_link_libraries(pipe_tool codectk_static m)

# pkg-config file
configure_file(codectk.pc.in codectk.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/codectk.pc DESTINATION lib/pkgconfig)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C flags: ${CMAKE_C_FLAGS}")
