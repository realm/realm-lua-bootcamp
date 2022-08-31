cmake_minimum_required(VERSION 3.20)
project(realm-lua)

set(LUA_ENABLE_SHARED OFF)
set(LUA_BUILD_COMPILER OFF)
add_subdirectory(external/lua)

set(REALM_BUILD_LIB_ONLY YES)
add_subdirectory(external/realm-core)

add_subdirectory(src)
