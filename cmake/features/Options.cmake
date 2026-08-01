option(GENERATE_DEBUG_SYMBOLS "Add the flags for generating debug symbols"  OFF)
option(ENABLE_FRAME_POINTER "Do not omit frame pointer" OFF)

if(GENERATE_DEBUG_SYMBOLS)
    if(MSVC)
        add_compile_options(/Zi)
        add_link_options(/DEBUG)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-g)
        add_link_options(-g)
    endif()
endif()

if(ENABLE_FRAME_POINTER)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fno-omit-frame-pointer)
    endif()
endif()

