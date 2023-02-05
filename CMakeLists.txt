cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
 
project(bencode)

option(BUILD_TESTS "Build tests" OFF)

set (HEADERS
  include/bencode/bencode.hpp
)

set(SOURCES
  src/decode.cpp
)

add_library(bencode ${SOURCES} ${HEADERS})

if(MSVC)
  target_compile_options(bencode PRIVATE /W4 /WX)
else()
  target_compile_options(bencode PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_include_directories(bencode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(external/googletest)
  add_subdirectory(tests)
endif()