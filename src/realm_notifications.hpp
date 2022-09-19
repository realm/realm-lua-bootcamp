#include <lua.hpp>

// Note: Needs to be defined before realm.h
struct realm_lua_userdata {
    lua_State* L;
    int callback_reference;
};

#define realm_userdata_t realm_lua_userdata*

#include <realm.h>

void populate_lua_object_changes_table(lua_State* L, int table_index, const char* field_name, realm_property_key_t* changes_properties, size_t changes_properties_size);

void on_object_change(realm_lua_userdata* userdata, const realm_object_changes_t* changes);

void populate_lua_collection_changes_table(lua_State* L, int table_index, const char* field_name, size_t* changes_indices, size_t changes_indices_size);

void on_collection_change(realm_lua_userdata* userdata, const realm_collection_changes_t* changes);

// Get the callback reference from the registry and unreference it
// so that Lua can garbage collect it.
void free_userdata(realm_lua_userdata* userdata);
