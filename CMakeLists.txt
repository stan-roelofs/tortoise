cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
 
project(tortoise VERSION 0.1.0 LANGUAGES CXX)
configure_file(cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/tortoise/version.hpp)

option(BUILD_TESTS "Build tests" OFF)

set (HEADERS
  include/tortoise/bencode.hpp
  include/tortoise/connection.hpp
  include/tortoise/exceptions.hpp
  include/tortoise/metainfo.hpp
  include/tortoise/socket.hpp
  include/tortoise/tracker.hpp

  src/httprequest.hpp
  src/sha1.hpp
)

set(SOURCES
  src/bencode.cpp
  src/connection.cpp
  src/httprequest.cpp
  src/metainfo.cpp
  src/sha1.cpp
  src/socket.cpp
  src/tracker.cpp
)

add_library(tortoise ${SOURCES} ${HEADERS})
if (WIN32)
  target_link_libraries(tortoise ws2_32)
endif()

if(MSVC)
  target_compile_options(tortoise PRIVATE /W4 /WX)
else()
  target_compile_options(tortoise PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_include_directories(tortoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(external/googletest)
  add_subdirectory(tests)
endif()