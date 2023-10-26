cmake_minimum_required(VERSION 3.2)

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR} "*.c" "*.h")

add_library(PDCurses ${SOURCES})
target_include_directories(PDCurses
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)