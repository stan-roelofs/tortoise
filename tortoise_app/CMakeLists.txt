cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)

option(DEVELOPMENT_BUILD "Build development version" OFF)
 
project(tortoise)

set(HEADERS
  app.hpp
)

set(SOURCES
  app.cpp
  main.cpp
)

add_executable(tortoise_app ${HEADERS} ${SOURCES})
target_link_libraries(tortoise_app PRIVATE tortoise)

# TODO: remove this
if (DEVELOPMENT_BUILD)
  target_compile_options(tortoise_app PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
  target_link_options(tortoise_app PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
  message(STATUS "Development build enabled")
endif()