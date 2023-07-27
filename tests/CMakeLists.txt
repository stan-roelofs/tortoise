# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(SOURCES
    src/test_bencode.cpp
    src/test_ip_address.cpp
    src/test_metainfo.cpp
    src/test_peer_id.cpp
    src/test_tracker_list.cpp
    src/test_url.cpp
)

add_executable(tortoise_test ${SOURCES})
target_link_libraries(tortoise_test tortoise gtest_main)
target_include_directories(tortoise_test PRIVATE ${CMAKE_SOURCE_DIR}/tortoise)

include(GoogleTest)
gtest_discover_tests(tortoise_test)