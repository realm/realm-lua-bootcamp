set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(realm-lua-objects OBJECT
    realm_util.cpp
    realm_schema.cpp
    realm_native_lib.cpp
    realm_notifications.cpp
)

target_include_directories(realm-lua-objects PUBLIC
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(realm-lua-objects PUBLIC 
    ${LUA_LIBRARIES}
    Realm::ObjectStore 
    Realm::QueryParser
    RealmFFIStatic
)

add_library(realm-lua MODULE)
target_link_libraries(realm-lua PRIVATE realm-lua-objects)
set_target_properties(realm-lua PROPERTIES
    OUTPUT_NAME "realm"
    PREFIX ""
)

install(TARGETS realm-lua
    LIBRARY DESTINATION "lib"
)
