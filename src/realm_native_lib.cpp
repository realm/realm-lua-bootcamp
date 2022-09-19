#include <vector>
#include <iostream>
#include <realm/util/to_string.hpp>

#include "realm_native_lib.hpp"
struct realm_lua_userdata {
    lua_State* L;
    int callback_reference;
};

#define realm_userdata_t realm_lua_userdata*

#include <realm.h>
#include "realm_native_lib.hpp"
#include "realm_schema.hpp"
#include "realm_util.hpp"

static int _lib_realm_open(lua_State* L) {
    realm_t** realm = static_cast<realm_t**>(lua_newuserdata(L, sizeof(realm_t*)));
    luaL_setmetatable(L, RealmHandle);
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "schema");
    realm_schema_t* schema = _parse_schema(L);
    if (!schema) {
        return _inform_realm_error(L);
    }
    lua_pop(L, 1);

    realm_config_t* config = realm_config_new();
    realm_config_set_schema(config, schema);
    realm_release(schema);

    lua_getfield(L, 1, "path");
    luaL_checkstring(L, -1);
    realm_config_set_path(config, lua_tostring(L, -1));

    lua_getfield(L, 1, "schemaVersion");
    luaL_checkinteger(L, -1);
    realm_config_set_schema_version(config, lua_tointeger(L, -1));
    // TODO?: add ability to change this through config object? 
    realm_config_set_schema_mode(config, RLM_SCHEMA_MODE_SOFT_RESET_FILE); // delete realm file if there are schema conflicts

    // Pop both fields.
    lua_pop(L, 2);

    *realm = realm_open(config);
    realm_release(config);
    if (!*realm) {
        // Exception ocurred while trying to open realm
        return _inform_realm_error(L);
    }
    return 1;
}

static int _lib_realm_release(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    void** realm_handle = static_cast<void**>(lua_touserdata(L, -1));
    realm_release(*realm_handle);
    *realm_handle = nullptr;
    return 0;
}

static int _lib_realm_begin_write(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, -1);
    bool status = realm_begin_write(*realm);
    if (!status){
        // Exception ocurred while trying to start a write transaction
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_commit_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, -1);
    bool status = realm_commit(*realm);
    if (!status){
        // Exception ocurred while trying to commit a write transaction
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_cancel_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, -1);
    bool status = realm_rollback(*realm);
    if (!status){
        // Exception ocurred while trying to cancel a write transaction
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_object_create(lua_State* L) {
    // Create RealmObject handle with its metatable to return to Lua 
    realm_object_t** realm_object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
    luaL_setmetatable(L, RealmHandle);

    // Get arguments from stack
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    const char* class_name = lua_tostring(L, 2);

    // Get class key corresponding to the object we create
    realm_class_info_t class_info;
    bool found = false;
    if (!realm_find_class(*realm, class_name, &found, &class_info)) {
        // Exception occurred when fetching a class
        return _inform_realm_error(L);
    }
    if (!found) {
        return _inform_error(L, "Class %1 not found", class_name);
    }

    // Create object and feed it into the RealmObject handle
    *realm_object = realm_object_create(*realm, class_info.key); 
    if (!*realm_object) {
        // Exception ocurred when creating an object
        return _inform_realm_error(L);
    }
    return 1;
}

static int _lib_realm_set_value(lua_State* L) {
    // Get arguments from stack
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 2);
    const char* property_name = lua_tostring(L, 3);

    // Get the property to update based on its string representation
    std::optional<realm_property_info_t> property_info;
    if (!(property_info = get_property_info_by_name(L, *realm, *realm_object, property_name))){
        // Property info not found
        return 0;
    } 

    // Translate the lua value into corresponding realm value
    std::optional<realm_value> value;
    if (!(value = lua_to_realm_value(L, 4))){
        // No corresponding realm value found
        return 0;
    }

    if (!realm_set_value(*realm_object, property_info->key, *value, false)) {
        // Exception ocurred when setting value
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_get_value(lua_State* L) {
    // Get arguments from stack
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 2);
    const char* property_name = lua_tostring(L, 3);

    // Get the property to fetch from based on its string representation
    if (auto property_info = get_property_info_by_name(L, *realm, *realm_object, property_name)){
        // Fetch desired value
        realm_value_t out_value;
        if (!realm_get_value(*realm_object, property_info->key, &out_value)) {
            // Exception ocurred while trying to fetch value
            return _inform_realm_error(L);
        }

        // Push correct lua value based on Realm type
        return realm_to_lua_value(L, out_value);
    } else {
        // No value found
        return 0;
    }
}

static int _lib_realm_object_get_all(lua_State* L) {
    // Get arguments from stack
    realm_t **realm = (realm_t**)lua_touserdata(L, 1);
    const char* class_name = lua_tostring(L, 2);

    // Get class info containing the class key
    bool class_found = false;
    realm_class_info_t class_info;
    if (!realm_find_class(*realm, class_name, &class_found, &class_info)) {
        return _inform_realm_error(L);
    }

    if (!class_found) {
        return _inform_error(L, "Unable to find collection");
    }

    // Push result onto stack
    realm_results_t **result = static_cast<realm_results_t**>(lua_newuserdata(L, sizeof(realm_results_t*)));
    *result = realm_object_find_all(*realm, class_info.key);

    return 1;
}

static int _lib_realm_results_get(lua_State* L) {
    // Get arguments from stack
    realm_results_t **realm_results = (realm_results_t **)lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);
    
    // Setup return value which is a realm_object
    realm_object_t **object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));

    // Fetch object
    *object = realm_results_get_object(*realm_results, index);

    return 1;
}

static int _lib_realm_results_count(lua_State* L) {
    // Get argument from stack
    realm_results_t **realm_results = (realm_results_t**)lua_touserdata(L, 1);
    size_t count;
    bool status = realm_results_count(*realm_results, &count);
    if (!status) {
        return _inform_realm_error(L);
    };
    
    // TODO: size_t: typedef unsigned long size_t. (Change from lua_pushinteger?)
    lua_pushinteger(L, count);

    return 1;
}

static void populate_lua_collection_changes_table(lua_State* L, int table_index, const char* field_name, size_t* changes_indices, size_t changes_indices_size) {
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

static void on_collection_change(realm_lua_userdata* userdata, const realm_collection_changes_t* changes) {
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
        _inform_error(L, "Could not call the callback function.");
        return;
    }
}

static void free_userdata(realm_lua_userdata* userdata) {
    // Get the callback reference from the registry and unreference it
    // so that Lua can garbage collect it.
    luaL_unref(userdata->L, LUA_REGISTRYINDEX, userdata->callback_reference);
    delete userdata;
}

static int _lib_realm_results_add_listener(lua_State* L) {
    // Get 1st argument (results/collection) from stack
    realm_results_t** results = (realm_results_t**)lua_touserdata(L, 1);
    
    // Pop 2nd argument/top of stack (the Lua function) from the stack and save a
    // reference to it in the register. "callback_reference" is the register location.
    int callback_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    // Create a pointer to userdata for use in the callback that will be
    // invoked at a later time.
    realm_lua_userdata* userdata = new realm_lua_userdata;
    userdata->L = L;
    userdata->callback_reference = callback_reference;

    // Put the notification token on the stack.
    auto** notification_token = static_cast<realm_notification_token_t**>(lua_newuserdata(L, sizeof(realm_notification_token_t*)));
    *notification_token = realm_results_add_notification_callback(
        *results,
        userdata,
        free_userdata,
        nullptr,
        on_collection_change
    );

    // Set the metatable of the notification token (top of stack) to that
    // of RealmHandle in order for it to be released via __gc.
    luaL_setmetatable(L, RealmHandle);

    if (!*notification_token) {
        lua_pop(L, 1);
        return _inform_realm_error(L);
    }

    return 1;
}

static void populate_lua_object_changes_table(lua_State* L, int table_index, const char* field_name, realm_property_key_t* changes_properties, size_t changes_properties_size) {
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

static void on_object_change(realm_lua_userdata* userdata, const realm_object_changes_t* changes) {
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
        _inform_error(L, "Could not call the callback function.");
        return;
    }

    return;
}

static int _lib_realm_object_add_listener(lua_State* L) {
    // Get 1st argument (object) from the stack.
    realm_object_t** object = (realm_object_t**)lua_touserdata(L, 1);

    // Pop 2nd argument/top of stack (the Lua function) from the stack and save a
    // reference to it in the register. "callback_reference" is the register location.
    int callback_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    // Create a pointer to userdata for use in the callback that will be
    // invoked at a later time.
    realm_lua_userdata* userdata = new realm_lua_userdata;
    userdata->L = L;
    userdata->callback_reference = callback_reference;

    // Put the notification token on the stack.
    auto** notification_token = static_cast<realm_notification_token_t**>(lua_newuserdata(L, sizeof(realm_notification_token_t*)));
    *notification_token = realm_object_add_notification_callback(
        *object,
        userdata,
        free_userdata,
        nullptr,
        on_object_change
    );

    // Set the metatable of the notification token (top of stack) to that
    // of RealmHandle in order for it to be released via __gc.
    luaL_setmetatable(L, RealmHandle);

    if (!*notification_token) {
        lua_pop(L, 1);
        return _inform_realm_error(L);
    }

    return 1;
}

static realm_query_t* _lib_realm_query_parse(lua_State* L, realm_t *realm, const char* class_name, const char* query_string, size_t num_args, size_t lua_arg_offset) {
    // Value which keeps track of the start location of arguments on the stack
    int arg_index;

    // Setup 2d vector to contain all realm_arguments provided
    std::vector<realm_query_arg_t> args_vector;
    std::vector<std::vector<realm_value_t>> value;
    for(int index = 0; index < num_args; index++){
        std::vector<realm_value_t>& realm_values = value.emplace_back();
        arg_index = lua_arg_offset + index;
        std::optional<realm_value_t> value;
        if (!(value = lua_to_realm_value(L, arg_index))){
            // Unknown value provided
            return nullptr;
        }
        realm_values.emplace_back(*value);
        args_vector.emplace_back(realm_query_arg_t{
            .nb_args = 1,
            .is_list = false,
            .arg = realm_values.data(),
        });
    }
    // set array value here from vector

    // Get class key
    bool class_found = false;
    realm_class_info_t class_info;
    if (!realm_find_class(realm, class_name, &class_found, &class_info)) {
        _inform_realm_error(L);
        return nullptr;
    }
    if (!class_found) {
        _inform_error(L, "Unable to find collection");
        return nullptr;
    }

    return realm_query_parse(realm, class_info.key, query_string, num_args, args_vector.data()); 
}

static int _lib_realm_results_filter(lua_State *L){
    realm_results_t **unfiltered_result = (realm_results_t**)lua_touserdata(L, 1);
    realm_t **realm = (realm_t**)lua_touserdata(L, 2);
    const char* class_name = lua_tostring(L, 3);
    const char* query_string = lua_tostring(L, 4);
    size_t num_args = lua_tointeger(L, 5);
    size_t lua_arg_offset = 6;
    realm_query_t *query = _lib_realm_query_parse(L, *realm, class_name, query_string, num_args, lua_arg_offset);
    if (!query){
        return _inform_realm_error(L);
    }
    realm_results_t **result = static_cast<realm_results_t**>(lua_newuserdata(L, sizeof(realm_results_t*)));
    *result = realm_results_filter(*unfiltered_result, query);
    return 1;
}

static const luaL_Reg lib[] = {
  {"realm_open",                    _lib_realm_open},
  {"realm_release",                 _lib_realm_release},
  {"realm_begin_write",             _lib_realm_begin_write},
  {"realm_commit_transaction",      _lib_realm_commit_transaction},
  {"realm_cancel_transaction",      _lib_realm_cancel_transaction},
  {"realm_object_create",           _lib_realm_object_create},
  {"realm_set_value",               _lib_realm_set_value},
  {"realm_get_value",               _lib_realm_get_value},
  {"realm_object_get_all",          _lib_realm_object_get_all},
  {"realm_object_add_listener",     _lib_realm_object_add_listener},
  {"realm_results_get",             _lib_realm_results_get},
  {"realm_results_count",           _lib_realm_results_count},
  {"realm_results_add_listener",    _lib_realm_results_add_listener},
  {"realm_results_filter",          _lib_realm_results_filter},
  {NULL, NULL}
};

void realm_lib_open(lua_State* L) {
    // see linit.c from the Lua source code
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    lua_pushcfunction(L, [](lua_State* L) {
        luaL_newlib(L, lib);
        return 1;
    });
    lua_setfield(L, -2, "_realm_native");
    lua_pop(L, 1);

    luaL_newmetatable(L, RealmHandle);
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, &_lib_realm_release);
    lua_settable(L, -3);
}
