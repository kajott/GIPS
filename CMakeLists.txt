cmake_minimum_required (VERSION 3.10)

project (GIPS)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

add_executable (gips
    src/libs.c
    src/main.c
    thirdparty/glad/src/glad.c
)

target_include_directories (gips PRIVATE
    src
    thirdparty/sokol
    thirdparty/imgui
    thirdparty/stb
    thirdparty/glad/include
)

if (WIN32)
    target_link_libraries (gips opengl32)
else ()
    target_link_libraries (gips pthread m dl X11 Xi Xcursor GL)
endif ()
