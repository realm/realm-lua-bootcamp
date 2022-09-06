set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_executable(realm-lua-playground
    main.cpp
    realm_native_lib.cpp
)

target_compile_definitions(realm-lua-playground PUBLIC
    SCRIPT_SOURCE_PATH="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(realm-lua-playground 
    Lua::Library 
    Realm::ObjectStore 
    Realm::QueryParser
    RealmFFIStatic
)