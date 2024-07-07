cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
 
project(Tortoise VERSION 0.1.0 LANGUAGES CXX)
configure_file(cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/tortoise/version.hpp)

option(TORTOISE_ENABLE_LOGGING "Enable logging" OFF)
option(TORTOISE_BUILD_APP "Build application" ON)
option(TORTOISE_BUILD_TESTS "Build tests" ON)

set (HEADERS
  include/tortoise/event.hpp
  include/tortoise/exception.hpp
  include/tortoise/exceptions.hpp
  include/tortoise/ip_address.hpp
  include/tortoise/load_torrent.hpp
  include/tortoise/metainfo.hpp
  include/tortoise/session.hpp
  include/tortoise/sha1_hash.hpp
  include/tortoise/torrent.hpp
  include/tortoise/version.hpp

  tortoise/bencode.hpp
  tortoise/event_queue.hpp
  tortoise/peer.hpp
  tortoise/peer_id.hpp
  tortoise/network.hpp
  tortoise/peer_info.hpp
  tortoise/piece_manager.hpp
  tortoise/tracker/http_tracker_connection.hpp
  tortoise/tracker/tracker_announce.hpp
  tortoise/tracker/tracker_connection.hpp
  tortoise/tracker/tracker_list.hpp
  tortoise/tracker/tracker_manager.hpp
  tortoise/tracker/udp_tracker_connection.hpp
  tortoise/sha1.hpp
  tortoise/socket.hpp
  tortoise/torrent_impl.hpp
  tortoise/url.hpp
  tortoise/util.hpp
)

set(SOURCES
  tortoise/bencode.cpp
  tortoise/event_queue.cpp
  tortoise/ip_address.cpp
  tortoise/load_torrent.cpp
  tortoise/peer.cpp
  tortoise/peer_id.cpp
  tortoise/piece_manager.cpp
  tortoise/sha1_hash.cpp
  tortoise/sha1.cpp
  tortoise/session.cpp
  tortoise/socket.cpp
  tortoise/torrent_impl.cpp
  tortoise/tracker/http_tracker_connection.cpp
  tortoise/tracker/tracker_announce.cpp
  tortoise/tracker/tracker_connection.cpp
  tortoise/tracker/tracker_list.cpp
  tortoise/tracker/tracker_manager.cpp
  tortoise/tracker/udp_tracker_connection.cpp
  tortoise/url.cpp
)

add_library(tortoise ${SOURCES} ${HEADERS})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(tortoise PRIVATE Threads::Threads)

if (WIN32)
  target_link_libraries(tortoise PRIVATE ws2_32)
endif()

if (TORTOISE_ENABLE_LOGGING)
  message("Logging enabled")
  target_compile_definitions(tortoise PRIVATE -DTORTOISE_ENABLE_LOGGING)
endif()
if(MSVC)
  target_compile_definitions(tortoise PRIVATE NOMINMAX)
  target_compile_options(tortoise PRIVATE /W4 /WX)
else()
  target_compile_options(tortoise PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_include_directories(tortoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tortoise)

if(TORTOISE_BUILD_TESTS)
  enable_testing()
  set(INSTALL_GTEST, OFF)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory(external/googletest)
  add_subdirectory(tests)
endif()

if (TORTOISE_BUILD_APP)
  if (WIN32)
    add_subdirectory(external/PDCurses)
  endif()
  add_subdirectory(tortoise_app)
endif()
