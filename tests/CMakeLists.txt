# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(SOURCES
    src/test_decode.cpp
)

add_executable(bencode_test ${SOURCES})
target_link_libraries(bencode_test bencode gtest_main)

include(GoogleTest)
gtest_discover_tests(bencode_test)