cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)
 
project(Tortoise VERSION 0.2.0 LANGUAGES CXX)
configure_file(cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/tortoise/version.hpp)

option(TORTOISE_BUILD_APP "Build application" ON)
option(TORTOISE_BUILD_TESTS "Build tests" ON)

set (HEADERS
  include/tortoise/event.hpp
  include/tortoise/exception.hpp
  include/tortoise/load_torrent.hpp
  include/tortoise/logging.hpp
  include/tortoise/metainfo.hpp
  include/tortoise/peer_id.hpp
  include/tortoise/peer_info.hpp
  include/tortoise/session.hpp
  include/tortoise/torrent.hpp
  include/tortoise/version.hpp

  tortoise/event_queue.hpp

  tortoise/bencode/bencode.hpp

  tortoise/torrent/bitfield.hpp
  tortoise/torrent/peer.hpp
  tortoise/torrent/peer_connection.hpp
  tortoise/torrent/piece_manager.hpp
  tortoise/torrent/piece_writer.hpp
  tortoise/torrent/torrent_impl.hpp

  tortoise/tracker/announce.hpp
  tortoise/tracker/connection.hpp
  tortoise/tracker/tiered_list.hpp
  tortoise/tracker/manager.hpp

  tortoise/network/ip_address.hpp
  tortoise/network/socket.hpp
  tortoise/network/url.hpp

  tortoise/util/log.hpp
  tortoise/util/hash.hpp
  tortoise/util/util.hpp
)

set(SOURCES
  tortoise/session.cpp

  tortoise/bencode/bencode.cpp

  tortoise/network/ip_address.cpp
  tortoise/network/socket.cpp
  tortoise/network/url.cpp

  tortoise/torrent/bitfield.cpp
  tortoise/torrent/load_torrent.cpp
  tortoise/torrent/peer.cpp
  tortoise/torrent/peer_connection.cpp
  tortoise/torrent/peer_id.cpp
  tortoise/torrent/piece_manager.cpp
  tortoise/torrent/piece_writer.cpp
  tortoise/torrent/torrent_impl.cpp

  tortoise/tracker/http_connection.cpp
  tortoise/tracker/announce.cpp
  tortoise/tracker/tiered_list.cpp
  tortoise/tracker/manager.cpp
  tortoise/tracker/udp_connection.cpp

  tortoise/util/hash.cpp
  tortoise/util/log.cpp
)

add_library(tortoise ${SOURCES} ${HEADERS})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(tortoise PRIVATE Threads::Threads)

if (WIN32)
  target_link_libraries(tortoise PRIVATE ws2_32)
endif()

if(MSVC)
  target_compile_definitions(tortoise PRIVATE NOMINMAX)
  target_compile_options(tortoise PRIVATE /W4 /WX /wd4100 /wd4101)
else()
  target_compile_options(tortoise PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_include_directories(tortoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(TORTOISE_BUILD_TESTS)
  enable_testing()
  set(INSTALL_GTEST, OFF)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory(external/googletest)
  add_subdirectory(tests)
endif()

if (TORTOISE_BUILD_APP)
  add_subdirectory(tortoise_app)
endif()
