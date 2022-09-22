#include "realm_notifications.hpp"
#include "realm_util.hpp"

// Get the callback reference from the registry and unreference it
// so that Lua can garbage collect it.
void free_userdata(realm_lua_userdata* userdata) {
    luaL_unref(userdata->L, LUA_REGISTRYINDEX, userdata->callback_reference);
    delete userdata;
}

void populate_lua_object_changes_table(lua_State* L, int table_index, const char* field_name, realm_property_key_t* changes_properties, size_t changes_properties_size) {
    // Push an empty table onto the stack acting as a Lua array.
    lua_newtable(L);
    for (size_t index = 0; index < changes_properties_size; index++) {
        // Get the property key of the changed object (changes_properties[index])
        // and push onto stack.
        lua_pushinteger(L, changes_properties[index]);
        // Add the above property key to the table/array at position "index + 1"
        // and pop it from the stack.
        lua_rawseti(L, -2, index + 1);
    }

    // Set the above array (top of the stack) to be the value of
    // the field_name key on the table and pop from the stack.
    // (Table: { <field_name>: <changes_properties> })
    lua_setfield(L, table_index, field_name);
}

void on_object_change(realm_lua_userdata* userdata, const realm_object_changes_t* changes) {
    // Get the modified properties only if the object was not deleted.
    size_t num_modified_properties = realm_object_changes_get_num_modified_properties(changes);
    realm_property_key_t modified_properties[num_modified_properties];
    bool object_is_deleted = realm_object_changes_is_deleted(changes);
    if (!object_is_deleted) {
        realm_object_changes_get_modified_properties(changes, modified_properties, num_modified_properties);
    }

    lua_State* L = userdata->L;

    // Get the Lua callback function from the register and put onto the stack.
    lua_rawgeti(L, LUA_REGISTRYINDEX, userdata->callback_reference);

    // Push a new table onto the stack and add isDeleted and modifiedProperties
    // ({ isDeleted: <bool>, modifiedProperties: <modified_properties> }).
    lua_newtable(L);
    int table_index = lua_gettop(L);
    lua_pushboolean(L, object_is_deleted);
    lua_setfield(L, table_index, "isDeleted");
    populate_lua_object_changes_table(L, table_index, "modifiedProperties", modified_properties, num_modified_properties);

    // Call the callback function with the above table (top of stack) as the 1 argument.
    int status = lua_pcall(L, 1, 0, 0);
    if (status != LUA_OK) {
        _inform_error(L, "Could not call the callback function:\n%1", lua_tostring(L, -1));
        return;
    }

    return;
}

void populate_lua_collection_changes_table(lua_State* L, int table_index, const char* field_name, size_t* changes_indices, size_t changes_indices_size) {
    // Push an empty table onto the stack acting as a Lua array.
    lua_newtable(L);
    for (size_t index = 0; index < changes_indices_size; index++) {
        // Get the index of the changed object (changes_indices[index])
        // and convert to Lua's 1-based index (+1) and push onto stack.
        lua_pushinteger(L, changes_indices[index] + 1);
        // Add the above index value to the table/array at position "index + 1"
        // and pop it from the stack.
        lua_rawseti(L, -2, index + 1);
    }

    // Set the above array (top of the stack) to be the value of
    // the field_name key on the table and pop from the stack.
    // (Table: { <field_name>: <changes_indices> })
    lua_setfield(L, table_index, field_name);
}

void on_collection_change(realm_lua_userdata* userdata, const realm_collection_changes_t* changes) {
    size_t num_deletions;
    size_t num_insertions;
    size_t num_modifications;
    realm_collection_changes_get_num_changes(
        changes,
        &num_deletions,
        &num_insertions,
        &num_modifications,
        nullptr
    );

    size_t deletions_indices[num_deletions];
    size_t insertions_indices[num_insertions];
    size_t modifications_indices_old[num_modifications];
    size_t modifications_indices_new[num_modifications];
    realm_collection_changes_get_changes(
        changes,
        deletions_indices,
        num_deletions,
        insertions_indices,
        num_insertions,
        modifications_indices_old,
        num_modifications,
        modifications_indices_new,
        num_modifications,
        nullptr,
        0
    );

    lua_State* L = userdata->L;

    // Get the Lua callback function from the register and push onto the stack.
    lua_rawgeti(L, LUA_REGISTRYINDEX, userdata->callback_reference);

    // Push a new table onto the stack and add the indices arrays to the corresponding
    // keys (e.g. { deletions: <deletions_indices>, insertions: <insertion_indices> }).
    lua_newtable(L);
    int table_index = lua_gettop(L);
    populate_lua_collection_changes_table(L, table_index, "deletions", deletions_indices, num_deletions);
    populate_lua_collection_changes_table(L, table_index, "insertions", insertions_indices, num_insertions);
    populate_lua_collection_changes_table(L, table_index, "modificationsOld", modifications_indices_old, num_modifications);
    populate_lua_collection_changes_table(L, table_index, "modificationsNew", modifications_indices_new, num_modifications);

    // Call the callback function with the above table (top of stack) as the 1 argument.
    int status = lua_pcall(L, 1, 0, 0);
    if (status != LUA_OK) {
        _inform_error(L, "Could not call the callback function:\n%1", lua_tostring(L, -1));
        return;
    }
}
