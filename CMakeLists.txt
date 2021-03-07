cmake_minimum_required (VERSION 3.10)

project (GIPS)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

add_executable (gips
    src/libs.c
    src/main.c
)

target_include_directories (gips PRIVATE
    src
    thirdparty/sokol
    thirdparty/imgui
    thirdparty/stb
)

set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
target_link_libraries (gips Threads::Threads)

if (WIN32)
    target_link_libraries (gips opengl32)
else ()
    target_link_libraries (gips m dl X11 Xi Xcursor GL)
endif ()
