cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
 
project(Tortoise VERSION 0.1.0 LANGUAGES CXX)
configure_file(cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/tortoise/version.hpp)

option(TORTOISE_ENABLE_LOGGING "Enable logging" ON)
option(TORTOISE_BUILD_APP "Build application" ON)
option(TORTOISE_BUILD_TESTS "Build tests" ON)

if (TORTOISE_ENABLE_LOGGING)
  add_definitions(-DTORTOISE_ENABLE_LOGGING)
endif()

set (HEADERS
  include/tortoise/bencode.hpp
  include/tortoise/exceptions.hpp
  include/tortoise/http/exception.hpp
  include/tortoise/http/request.hpp
  include/tortoise/http/response.hpp
  include/tortoise/ip_address.hpp
  include/tortoise/load_torrent.hpp
  include/tortoise/metainfo.hpp
  include/tortoise/peer_id.hpp
  include/tortoise/session.hpp
  include/tortoise/sha1_hash.hpp
  include/tortoise/socket.hpp
  include/tortoise/torrent.hpp
  include/tortoise/tracker.hpp
  include/tortoise/tracker_manager.hpp
  include/tortoise/url.hpp
  include/tortoise/version.hpp
  lib/sha1.hpp
  lib/socket_helper.hpp
)

set(SOURCES
  lib/bencode.cpp
  lib/http/request.cpp
  lib/http/response.cpp
  lib/ip_address.cpp
  lib/load_torrent.cpp
  lib/metainfo.cpp
  lib/peer_id.cpp
  lib/sha1_hash.cpp
  lib/sha1.cpp
  lib/session.cpp
  lib/socket.cpp
  lib/socket_helper.cpp
  lib/torrent.cpp
  lib/tracker.cpp
  lib/tracker_manager.cpp
  lib/url.cpp
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

if(TORTOISE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(external/googletest)
  add_subdirectory(tests)
endif()

if (TORTOISE_BUILD_APP)
  add_subdirectory(src)
endif()