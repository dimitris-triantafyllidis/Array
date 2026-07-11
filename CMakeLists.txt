cmake_minimum_required(VERSION 3.22)

project (
    Array
    VERSION 1.0
    LANGUAGES CXX
)

set ( CMAKE_CXX_STANDARD           23 )
set ( CMAKE_CXX_STANDARD_REQUIRED  ON )
set ( CMAKE_CXX_EXTENSIONS        OFF )

# Header library

add_library ( array INTERFACE )
add_library ( Array::array ALIAS array )

target_include_directories ( array
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Example program

if ( PROJECT_IS_TOP_LEVEL )

    add_executable ( array_example
        ${CMAKE_CURRENT_SOURCE_DIR}/source/example.cpp
    )

    target_link_libraries ( array_example
        PRIVATE
            Array::array
    )

endif()

