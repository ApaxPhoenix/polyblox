cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(runtime C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(ExternalProject)

find_package(Vulkan REQUIRED)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sol3/CMakeLists.txt")
    file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sol3")
    execute_process(COMMAND git clone --branch v3.3.0 https://github.com/ThePhD/sol2.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sol3")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/yoga/CMakeLists.txt")
    file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/yoga")
    execute_process(COMMAND git clone --branch v3.1.0 https://github.com/facebook/yoga.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/yoga")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/metal-cpp/README.md")
    file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/metal-cpp")
    execute_process(COMMAND git clone https://github.com/bkaradzic/metal-cpp.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/metal-cpp")
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luajit-2.1")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/src")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib")

if(MSVC)
    ExternalProject_Add(luajit
            PREFIX "${CMAKE_BINARY_DIR}/vendor/luajit"
            GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
            GIT_TAG v2.1
            SOURCE_DIR "${CMAKE_BINARY_DIR}/luajit-src"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            UPDATE_COMMAND ""
            BUILD_COMMAND cmd /c src\\msvcbuild.bat static
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy src/lua51.lib "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/lua51.lib" && ${CMAKE_COMMAND} -E copy src/lua.h src/lualib.h src/lauxlib.h src/luaconf.h src/lua.hpp src/luajit.h "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/src/"
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/lua51.lib"
    )
elseif(WIN32)
    find_package(Git QUIET)
    set(LUAJIT_BUILD_CMD mingw32-make HOST_CC=gcc CROSS= PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit BUILDMODE=static CFLAGS=-fPIC)
    set(LUAJIT_INSTALL_CMD mingw32-make install PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit)

    if(GIT_FOUND)
        get_filename_component(GIT_DIR "${GIT_EXECUTABLE}" DIRECTORY)
        if(EXISTS "${GIT_DIR}/../usr/bin/sh.exe")
            get_filename_component(SH_DIR "${GIT_DIR}/../usr/bin" REALPATH)
        elseif(EXISTS "${GIT_DIR}/../bin/sh.exe")
            get_filename_component(SH_DIR "${GIT_DIR}/../bin" REALPATH)
        endif()
        if(SH_DIR)
            file(TO_NATIVE_PATH "${SH_DIR}" SH_DIR_NATIVE)
            string(REPLACE ";" "$<SEMICOLON>" EXPANDED_PATH "$ENV{PATH}")
            set(LUAJIT_BUILD_CMD ${CMAKE_COMMAND} -E env "PATH=${SH_DIR_NATIVE}$<SEMICOLON>${EXPANDED_PATH}" ${LUAJIT_BUILD_CMD})
            set(LUAJIT_INSTALL_CMD ${CMAKE_COMMAND} -E env "PATH=${SH_DIR_NATIVE}$<SEMICOLON>${EXPANDED_PATH}" ${LUAJIT_INSTALL_CMD})
        endif()
    endif()

    ExternalProject_Add(luajit
            PREFIX "${CMAKE_BINARY_DIR}/vendor/luajit"
            GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
            GIT_TAG v2.1
            SOURCE_DIR "${CMAKE_BINARY_DIR}/luajit-src"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            UPDATE_COMMAND ""
            BUILD_COMMAND ${LUAJIT_BUILD_CMD}
            INSTALL_COMMAND ${LUAJIT_INSTALL_CMD}
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
    )
elseif(APPLE)
    ExternalProject_Add(luajit
            PREFIX "${CMAKE_BINARY_DIR}/vendor/luajit"
            GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
            GIT_TAG v2.1
            SOURCE_DIR "${CMAKE_BINARY_DIR}/luajit-src"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            UPDATE_COMMAND ""
            BUILD_COMMAND env MACOSX_DEPLOYMENT_TARGET=11.0 make -j4 PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit BUILDMODE=static CFLAGS=-fPIC
            INSTALL_COMMAND env MACOSX_DEPLOYMENT_TARGET=11.0 make install PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
    )
else()
    ExternalProject_Add(luajit
            PREFIX "${CMAKE_BINARY_DIR}/vendor/luajit"
            GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
            GIT_TAG v2.1
            SOURCE_DIR "${CMAKE_BINARY_DIR}/luajit-src"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            UPDATE_COMMAND ""
            BUILD_COMMAND make -j4 PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit BUILDMODE=static CFLAGS=-fPIC
            INSTALL_COMMAND make install PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
    )
endif()

add_library(liblua STATIC IMPORTED GLOBAL)
if(MSVC)
    set_target_properties(liblua PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/lua51.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/src"
    )
else()
    set_target_properties(liblua PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luajit-2.1"
    )
endif()
add_dependencies(liblua luajit)

add_library(sol3 INTERFACE)
target_include_directories(sol3 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sol3/include")

file(GLOB_RECURSE YOGA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/yoga/yoga/*.cpp"
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

if(APPLE)
    file(GLOB_RECURSE METAL_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
    )
    list(APPEND SOURCES ${METAL_SOURCES})
else()
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/library/drivers/metal.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

add_library(runtime SHARED ${SOURCES} ${HEADERS} ${YOGA_SOURCES})
add_dependencies(runtime liblua)

target_compile_definitions(runtime PRIVATE
        SOL_NO_CHECK_NUMBER_TYPE=1
        SOL_ALL_SAFETIES_ON=0
)

if(WIN32 AND NOT MSVC)
    target_compile_options(runtime PRIVATE -Wa,-mbig-obj)
endif()

set_target_properties(runtime PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
        PREFIX ""
        OUTPUT_NAME "runtime"
)

target_include_directories(runtime PUBLIC
        include
        src
        src/vm/api
        src/vm/core
        vendor/yoga
        vendor/yoga/yoga
        vendor/metal-cpp
        ${Vulkan_INCLUDE_DIRS}
)

if(MSVC)
    target_link_libraries(runtime PRIVATE sol3 liblua Vulkan::Vulkan d3d12 dxgi)
    target_link_options(runtime PRIVATE
            /WHOLEARCHIVE:$<TARGET_FILE:liblua>
    )
elseif(WIN32)
    target_link_libraries(runtime PRIVATE sol3 liblua Vulkan::Vulkan d3d12 dxgi m pthread)
    target_link_options(runtime PRIVATE
            -Wl,--whole-archive
            $<TARGET_FILE:liblua>
            -Wl,--no-whole-archive
    )
elseif(APPLE)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(QUARTZ_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(runtime PRIVATE sol3 liblua Vulkan::Vulkan ${METAL_FRAMEWORK} ${QUARTZ_FRAMEWORK} m dl pthread)
    target_link_options(runtime PRIVATE
            -Wl,-force_load,$<TARGET_FILE:liblua>
    )
else()
    target_link_libraries(runtime PRIVATE sol3 liblua Vulkan::Vulkan m dl pthread)
    target_link_options(runtime PRIVATE
            -Wl,--whole-archive
            $<TARGET_FILE:liblua>
            -Wl,--no-whole-archive
    )
endif()