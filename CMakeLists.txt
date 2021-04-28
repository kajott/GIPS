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
message (STATUS "Compiler Type: ${CMAKE_CXX_COMPILER_ID}")

# set C/C++ language standards for compilers that respect them
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# use the static C library with MSVC builds
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# add the GLFW library with suitable options
foreach (disable_ GLFW_BUILD_EXAMPLES GLFW_BUILD_TESTS GLFW_BUILD_DOCS GLFW_INSTALL)
    option ("${disable_}" OFF)
endforeach ()
add_subdirectory (thirdparty/glfw)

# set sources for main program and third-party libs
add_executable (gips
    src/main.cpp
    src/gips_app.cpp
    src/gips_ui.cpp
    src/gips_paths.cpp
    src/gips_core.cpp
    src/gips_shader_loader.cpp
    src/gl_util.cpp
    src/string_util.cpp
    src/vfs.cpp
    src/patterns.cpp
    src/git_rev.c
    src/sysinfo.cpp
)
add_library (gips_thirdparty STATIC
    thirdparty/glad/src/glad.c
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/backends/imgui_impl_glfw.cpp
    thirdparty/imgui/backends/imgui_impl_opengl3.cpp
    src/libs_c.c
    src/libs_cpp.cpp
)

# set include directories
target_include_directories (gips_thirdparty PUBLIC
    src
    thirdparty/glad/include
    thirdparty/imgui
    thirdparty/imgui/backends
    thirdparty/stb
    thirdparty/pfd
)
target_include_directories (gips PRIVATE src)

# set library dependencies
target_link_libraries (gips_thirdparty glfw)
target_link_libraries (gips gips_thirdparty glfw)
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
if (NOT MSVC)
    target_compile_options (gips PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (gips PRIVATE /W4 /WX)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # the following option is useful for code quality testing only;
    # due to -Werror, no functional builds will be produced, as there are
    # *always* some warnings in third-party headers
#    target_compile_options (gips PRIVATE -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT MSVC)
        message (STATUS "Debug build, enabling Address Sanitizer")
        target_compile_options (gips PRIVATE "-fsanitize=address")
        target_compile_options (gips_thirdparty PUBLIC "-fsanitize=address")
        target_link_options (gips PRIVATE "-fsanitize=address")
    elseif (MSVC_VERSION GREATER 1627 AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message (STATUS "Debug build and MSVC 16.8 or greater detected, enabling Address Sanitizer")
        target_compile_options (gips PRIVATE "/fsanitize=address")
        target_compile_options (gips_thirdparty PUBLIC "/fsanitize=address")
        target_link_options (gips PRIVATE "/DEBUG")
        # ASAN isn't compatible with the /RTC switch and incremental linking,
        # both of which CMake enables by default
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string (REGEX REPLACE "/INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    endif ()
endif ()

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

# set install locations and generate packages
file (READ src/gips_version.h VERSION_H)
string (REGEX MATCH "[0-9]+\.[^\\\"]*" CPACK_PACKAGE_VERSION "${VERSION_H}")
string (REGEX MATCH "[0-9]+" CPACK_PACKAGE_VERSION_MAJOR "${CPACK_PACKAGE_VERSION}")
string (REGEX MATCH "\.([0-9]+)" MINOR_WITH_DOT "${CPACK_PACKAGE_VERSION}")
string (SUBSTRING "${MINOR_WITH_DOT}" 1 -1 CPACK_PACKAGE_VERSION_MINOR)
string (TIMESTAMP TODAY "%Y%m%d")
string (REGEX REPLACE "-((WIP|alpha|beta|rc|pre).*)" "~\\1-${TODAY}" CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
string (REPLACE "~" "-" CPACK_PACKAGE_VERSION "${CPACK_DEBIAN_PACKAGE_VERSION}")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "GLSL Image Processing System")
set (CPACK_PACKAGE_DESCRIPTION "GIPS is a simple image processing tool based purely on GLSL shaders.")
set (CPACK_PACKAGE_CONTACT "Martin J. Fiedler <keyj@emphy.de>")
set (CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_LIST_DIR}")
if (UNIX)
    install (TARGETS gips RUNTIME)
    install (FILES LICENSE.txt README.md ShaderFormat.md DESTINATION share/doc/gips)
    install (DIRECTORY shaders DESTINATION share/gips)
    set (CPACK_GENERATOR "TXZ;DEB;RPM")
    set (CPACK_PACKAGE_NAME "gips")
    set (CPACK_STRIP_FILES TRUE)
    set (CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1")
    set (CPACK_DEBIAN_PACKAGE_RECOMMENDS "zenity | kdialog")
    set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
    set (CPACK_DEBIAN_COMPRESSION_TYPE "xz")
    set (CPACK_RPM_COMPRESSION_TYPE "xz")
    set (CPACK_RPM_PACKAGE_LICENSE "MIT")
    set (CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set (CPACK_RPM_PACKAGE_GROUP "Amusements/Graphics")
    set (CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    set (CPACK_RPM_FILE_NAME "RPM-DEFAULT")
elseif (WIN32)
    string (REPLACE "\\" "/" CMAKE_INSTALL_PREFIX "$ENV{ProgramW6432}/GIPS")
    install (TARGETS gips DESTINATION .)
    install (FILES LICENSE.txt README.md ShaderFormat.md DESTINATION .)
    install (DIRECTORY shaders DESTINATION .)
    set (CPACK_GENERATOR "NSIS64")
    set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE.txt")
    set (CPACK_PACKAGE_NAME "GIPS")
    set (CPACK_PACKAGE_INSTALL_DIRECTORY "GIPS")
    set (CPACK_NSIS_DISPLAY_NAME "GIPS")
    set (CPACK_NSIS_PACKAGE_NAME "GIPS")
    set (CPACK_PACKAGE_EXECUTABLES "gips;GIPS")
    set (CPACK_NSIS_EXECUTABLES_DIRECTORY .)
endif ()
include (CPack)
