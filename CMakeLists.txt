cmake_minimum_required (VERSION 3.10)

project (GIPS)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# ensure we have a working SDL2 library; on Win32,
# bootstrap the development libraries ourself
if (WIN32)
    set (SDL2_VERSION 2.0.14)
    set (SDL2_DIR "${CMAKE_CURRENT_LIST_DIR}/thirdparty/SDL2-${SDL2_VERSION}")
    if (NOT EXISTS "${SDL2_DIR}")
        message (STATUS "${SDL2_DIR} does not exist")
        set (SDL2_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/thirdparty/SDL2-devel-${SDL2_VERSION}-VC.zip")
        if (NOT EXISTS "${SDL2_ARCHIVE}")
            message (STATUS "downloading SDL development headers ...")
            file (DOWNLOAD http://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-VC.zip "${SDL2_ARCHIVE}")
        endif ()
        message (STATUS "unpacking SDL development headers ...")
        execute_process (COMMAND powershell Expand-Archive "${SDL2_ARCHIVE}" -DestinationPath "${CMAKE_CURRENT_LIST_DIR}/thirdparty")
    endif ()
    set (SDL2_LIB_DIR "${SDL2_DIR}/lib/${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}")
    set (SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
    set (SDL2_LIBRARIES "${SDL2_LIB_DIR}/SDL2.lib" "${SDL2_LIB_DIR}/SDL2main.lib")
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/SDL.dll")
        file (COPY "${SDL2_LIB_DIR}/SDL2.dll" DESTINATION "${CMAKE_CURRENT_LIST_DIR}")
    endif ()
else ()
    find_package (SDL2 REQUIRED)
endif ()

add_executable (gips
    src/libs.c
    src/main.cpp
    src/gips_app.cpp
    src/gl_util.cpp
    thirdparty/glad/src/glad.c
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/backends/imgui_impl_sdl.cpp
    thirdparty/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories (gips PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
    thirdparty/glad/include
    thirdparty/imgui
    thirdparty/imgui/backends
    thirdparty/stb
)

# make the binary appear in the project's root directory
set_target_properties (gips PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}"
)

# set libraries: -pthread, SDL2, and all the rest
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package (Threads REQUIRED)
target_link_libraries (gips Threads::Threads)
target_link_libraries (gips ${SDL2_LIBRARIES})
if (WIN32)
    target_link_libraries (gips opengl32)
else ()
    target_link_libraries (gips m dl GL)
endif ()
