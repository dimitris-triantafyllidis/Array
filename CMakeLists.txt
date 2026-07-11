cmake_minimum_required(VERSION 3.22)

project (
    ArrayProject
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable ( Array
    source/main.cpp
)
