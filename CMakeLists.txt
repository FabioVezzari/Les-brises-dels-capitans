cmake_minimum_required(VERSION 3.16)

project(les_brises_dels_capitans LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(les_brises
    src/main.cpp
    src/ship_class.cpp
    src/sim.cpp
    src/orders.cpp
    src/state.cpp
    src/weather.cpp
)


target_include_directories(les_brises PRIVATE
    include
    third_party
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(les_brises PRIVATE -Wall -Wextra -Wpedantic)
endif()
