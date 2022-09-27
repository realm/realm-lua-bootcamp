cmake_minimum_required(VERSION 3.20)
project(realm-lua)

if(NOT LUAROCKS)
    find_package(Lua REQUIRED)
endif()

set(REALM_BUILD_LIB_ONLY YES)
add_subdirectory(external/realm-core EXCLUDE_FROM_ALL)

find_package(CURL REQUIRED)

# Project-wide build flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

if(NOT LUAROCKS)
    add_subdirectory(playground)
endif()
