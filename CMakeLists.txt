cmake_minimum_required (VERSION 3.15)

project (GIPS)

# set default build type
set (default_build_type "Debug")
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set (CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "build type" FORCE)
        set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    endif ()
    message (STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
endif ()

# set C/C++ language standards for compilers that respect them
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# ensure we have a working SDL2 library; on Win32, bootstrap it ourselves
if (WIN32)
    # first, download the sources
    set (SDL2_VERSION 2.0.14)
    set (SDL2_DIR "${CMAKE_CURRENT_LIST_DIR}/thirdparty/SDL2-${SDL2_VERSION}")
    if (NOT EXISTS "${SDL2_DIR}/src")
        message (STATUS "${SDL2_DIR} does not exist")
        set (SDL2_ZIPFILE "SDL2-${SDL2_VERSION}.zip")
        set (SDL2_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/thirdparty/${SDL2_ZIPFILE}")
        if (NOT EXISTS "${SDL2_ARCHIVE}")
            message (STATUS "downloading SDL sources ...")
            file (DOWNLOAD "http://www.libsdl.org/release/${SDL2_ZIPFILE}" "${SDL2_ARCHIVE}")
        endif ()
        message (STATUS "unpacking SDL sources ...")
        execute_process (COMMAND powershell Expand-Archive "${SDL2_ARCHIVE}" -DestinationPath "${CMAKE_CURRENT_LIST_DIR}/thirdparty")
    endif ()
    # set options for a minimal static SDL2 build
    foreach (disable_
        SDL_AUDIO SDL_RENDER DIRECTX RENDER_D3D WASAPI
        SDL_ATOMIC SDL_CPUINFO SDL_FILE SDL_FILESYSTEM SDL_POWER SDL_LOCALE
        SDL_HAPTIC SDL_SENSOR
        ASSEMBLY MMX 3DNOW SSE SSE2 SSE3 SSEMATH
        VIDEO_OPENGLES VIDEO_VULKAN
        VIDEO_DUMMY DUMMYAUDIO DISKAUDIO
    )
        option ("${disable_}" OFF)
    endforeach ()
    set (SDL_SHARED OFF CACHE BOOL "" FORCE)
    set (SDL_STATIC ON  CACHE BOOL "" FORCE)
    # include SDL build directory and publish the required settings
    message (STATUS "v v v v v v v v v v v SDL2 configure messages begin here v v v v v v v v v v")
    add_subdirectory (${SDL2_DIR})
    message (STATUS "^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ SDL2 configure messages end here ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^")
    set (SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
    set (SDL2_LIBRARIES SDL2-static SDL2main)
else ()
    # on systems that tend to have proper package managers,
    # we can just use the system's default SDL2 installation
    find_package (SDL2 REQUIRED)
endif ()

# set sources for main program and third-party libs
add_executable (gips
    src/main.cpp
    src/gips_app.cpp
    src/gips_ui.cpp
    src/gips_core.cpp
    src/gips_shader_loader.cpp
    src/gl_util.cpp
    src/string_util.cpp
    src/dirlist.cpp
    src/patterns.cpp
    src/git_rev.c
)
add_library (gips_thirdparty STATIC
    thirdparty/glad/src/glad.c
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/backends/imgui_impl_sdl.cpp
    thirdparty/imgui/backends/imgui_impl_opengl3.cpp
    src/libs_c.c
    src/libs_cpp.cpp
)

# set include directories
target_include_directories (gips_thirdparty PUBLIC
    src
    ${SDL2_INCLUDE_DIRS}
    thirdparty/glad/include
    thirdparty/imgui
    thirdparty/imgui/backends
    thirdparty/stb
    thirdparty/pfd
)
target_include_directories (gips PRIVATE src)

# set library dependencies
target_link_libraries (gips gips_thirdparty ${SDL2_LIBRARIES})
if (WIN32)
    target_link_libraries (gips opengl32)
else ()
    target_link_libraries (gips m dl GL)
endif ()

# platform-dependent additional sources and options
if (WIN32)
    target_sources (gips PRIVATE
        src/file_util_win32.cpp
        src/clipboard_win32.cpp
        gips.manifest
    )
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties (gips PROPERTIES WIN32_EXECUTABLE ON)
    endif ()
else ()
    target_sources (gips PRIVATE
        src/file_util_posix.cpp
        src/clipboard_dummy.cpp
    )
    set (THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package (Threads REQUIRED)
    target_link_libraries (gips Threads::Threads)
endif ()

# compiler options
if(MSVC)
    target_compile_options(gips PRIVATE /W4 /WX)
else()
    target_compile_options(gips PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
endif()

# provide Git revision header file
add_custom_command (
    OUTPUT "${CMAKE_CURRENT_LIST_DIR}/src/git_rev.c"
           and_a_dummy_file_to_force_re-running_this_every_time
    COMMAND ${CMAKE_COMMAND}
            -P ${CMAKE_CURRENT_LIST_DIR}/version.cmake
)

# make the binary appear in the project's root directory
set_target_properties (gips PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_NAME_DEBUG                       "gips_debug"
)
