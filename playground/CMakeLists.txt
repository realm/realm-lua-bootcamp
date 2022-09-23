add_executable(realm-lua-playground main.cpp)

target_compile_definitions(realm-lua-playground PUBLIC
    SCRIPT_SOURCE_PATH="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(realm-lua-playground realm-lua-objects)
