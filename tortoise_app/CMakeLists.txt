cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
 
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