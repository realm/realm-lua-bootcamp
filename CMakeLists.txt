cmake_minimum_required(VERSION 3.20)
project(realm-lua)

set(LUA_ENABLE_SHARED OFF)
set(LUA_SUPPORT_DL ON)
add_subdirectory(external/lua EXCLUDE_FROM_ALL)

set(REALM_BUILD_LIB_ONLY YES)
add_subdirectory(external/realm-core EXCLUDE_FROM_ALL)

# Add libuv and tell Realm Core to use it
add_subdirectory(external/libuv EXCLUDE_FROM_ALL)
target_compile_definitions(ObjectStore PUBLIC
    REALM_HAVE_UV=1
    REALM_PLATFORM_NODE=1
)
target_link_libraries(ObjectStore PUBLIC uv_a)

# Project-wide build flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
