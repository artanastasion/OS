cmake_minimum_required(VERSION 3.10)
project(background_launcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(background STATIC
        background.cpp
        background.hpp
)

add_executable(background_test
        main.cpp
)

target_link_libraries(background_test background)