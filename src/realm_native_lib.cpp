#include <vector>
#include <iostream>

#include <realm/util/to_string.hpp>

// NOTE: Make sure to include realm_notifications before realm.h.
#include "realm_notifications.hpp"
#include <realm.h>
#include "realm_native_lib.hpp"
#include "realm_schema.hpp"
#include "realm_util.hpp"

static int lib_realm_open(lua_State* L) {
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

    realm_sync_config_t* sync_config = nullptr;
    lua_getfield(L, 1, "sync");
    if (lua_istable(L, -1)) {
        const char* partition_value;
        lua_getfield(L, -1, "partitionValue");
        if (lua_isstring(L, -1)) {
            partition_value = lua_tostring(L, -1);
        }
        else {
            return _inform_error(L, "sync.partitionValue must be a string, got %1.", lua_typename(L, lua_type(L, -1)));
        }
        lua_pop(L, 1);

        realm_user_t* user = nullptr;
        lua_getfield(L, -1, "user");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "_handle");
            if (auto** userdata = static_cast<realm_user_t**>(luaL_checkudata(L, -1, RealmHandle))) {
                user = *userdata;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        if (!user) {
            return _inform_error(L, "sync.user must be a User object.");
        }

        sync_config = realm_sync_config_new(user, partition_value);
        realm_config_set_sync_config(config, sync_config);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "path");
    if (lua_isstring(L, -1)) {
        realm_config_set_path(config, lua_tostring(L, -1));
    }
    else if (sync_config) {
        char* path = realm_app_sync_client_get_default_file_path_for_realm(sync_config, nullptr);
        realm_config_set_path(config, path);
        realm_free(path);
    }
    else {
        realm_config_set_path(config, "default.realm");
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "schemaVersion");
    if (lua_isinteger(L, -1)) {
        realm_config_set_schema_version(config, lua_tonumber(L, -1));
    }
    else {
        realm_config_set_schema_version(config, 0);
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "_cached");
    if (lua_isboolean(L, -1)) {
        realm_config_set_cached(config, lua_toboolean(L, -1));
    }
    lua_pop(L, 1);
    
    if (sync_config) {
        realm_config_set_schema_mode(config, RLM_SCHEMA_MODE_ADDITIVE_EXPLICIT);
    }
    else {
        // TODO?: Add ability to change this through config object? 
        realm_config_set_schema_mode(config, RLM_SCHEMA_MODE_SOFT_RESET_FILE); // Delete realm file if there are schema conflicts.
    }

    if (realm_scheduler_t** scheduler = static_cast<realm_scheduler_t**>(luaL_checkudata(L, 2, RealmHandle))) {
        realm_config_set_scheduler(config, *scheduler);
    }

    const realm_t** realm = static_cast<const realm_t**>(lua_newuserdata(L, sizeof(realm_t*)));
    luaL_setmetatable(L, RealmHandle);
    *realm = realm_open(config);
    realm_release(config);
    realm_release(sync_config);
    if (!*realm) {
        // Exception ocurred while trying to open realm.
        return _inform_realm_error(L);
    }
    _push_schema_info(L, *realm);

    return 2;
}

static int lib_realm_release(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    void** realm_handle = static_cast<void**>(lua_touserdata(L, -1));
    realm_release(*realm_handle);
    *realm_handle = nullptr;

    return 0;
}

static int lib_realm_begin_write(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, -1);
    bool status = realm_begin_write(*realm);
    if (!status) {
        // Exception ocurred while trying to start a write transaction.
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_commit_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, -1);
    bool status = realm_commit(*realm);
    if (!status) {
        // Exception ocurred while trying to commit a write transaction.
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_cancel_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, -1);
    bool status = realm_rollback(*realm);
    if (!status) {
        // Exception ocurred while trying to cancel a write transaction.
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_object_create(lua_State* L) {
    // Get arguments from the stack.
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    const int64_t class_key = lua_tointeger(L, 2);

    // Create and push a RealmObject onto the stack and set its metatable.
    realm_object_t** realm_object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
    luaL_setmetatable(L, RealmHandle);
    *realm_object = realm_object_create(*realm, class_key); 
    if (!*realm_object) {
        // Exception ocurred when creating an object.
        return _inform_realm_error(L);
    }

    return 1;
}

static int lib_realm_object_create_with_primary_key(lua_State* L) {
    // Get arguments from the stack.
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    const int class_key = lua_tointeger(L, 2);
    std::optional<realm_value_t> pk = lua_to_realm_value(L, 3);
    if (!pk) {
        // No corresponding realm value found.
        return 0;
    }

    // Create and push a RealmObject onto the stack and set its metatable.
    realm_object_t** realm_object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
    luaL_setmetatable(L, RealmHandle);
    *realm_object = realm_object_create_with_primary_key(*realm, class_key, *pk); 
    if (!*realm_object) {
        // Exception ocurred when creating an object.
        return _inform_realm_error(L);
    }

    return 1;
}

static int lib_realm_set_value(lua_State* L) {
    // Get arguments from the stack.
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 2);
    realm_property_key_t& property_key = *(static_cast<realm_property_key_t*>(lua_touserdata(L, 3)));

    // Get the 4th argument and convert its Lua value to its corresponding Realm value.
    std::optional<realm_value_t> value = lua_to_realm_value(L, 4);
    if (!value) {
        // No corresponding realm value found.
        return 0;
    }

    if (!realm_set_value(*realm_object, property_key, *value, false)) {
        // Exception ocurred when setting value.
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_get_value(lua_State* L) {
    // Get arguments from the stack.
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 2);
    realm_property_key_t& property_key = *(static_cast<realm_property_key_t*>(lua_touserdata(L, 3)));

    realm_value_t out_value;
    if (!realm_get_value(*realm_object, property_key, &out_value)) {
        // Exception ocurred while trying to fetch value.
        return _inform_realm_error(L);
    }

    // Convert the Realm value to its corresponding Lua value and push onto the stack.
    return realm_to_lua_value(L, *realm, out_value);
}

static int lib_realm_object_delete(lua_State* L) {
    realm_object_t **object = (realm_object_t**)lua_touserdata(L, 1);
    if (realm_object_delete(*object)) {
        return 0;
    }

    return _inform_realm_error(L);
}

static int lib_realm_object_is_valid(lua_State* L) {
    realm_object_t** object = (realm_object_t**)lua_touserdata(L, 1);
    lua_pushboolean(L, realm_object_is_valid(*object));

    return 1; 
}

static int lib_realm_object_get_all(lua_State* L) {
    // Get arguments from the stack.
    realm_t **realm = (realm_t**)lua_touserdata(L, 1);
    const char* class_name = lua_tostring(L, 2);

    // Get class info containing the class key.
    bool class_found = false;
    realm_class_info_t class_info;
    if (!realm_find_class(*realm, class_name, &class_found, &class_info)) {
        return _inform_realm_error(L);
    }

    if (!class_found) {
        return _inform_error(L, "Unable to find collection");
    }

    // Get and push the results onto the stack and set its metatable.
    realm_results_t** results = static_cast<realm_results_t**>(lua_newuserdata(L, sizeof(realm_results_t*)));
    luaL_setmetatable(L, RealmHandle);
    *results = realm_object_find_all(*realm, class_info.key);

    return 1;
}

static int lib_realm_results_get(lua_State* L) {
    // Get arguments from the stack.
    realm_results_t** realm_results = (realm_results_t**)lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);

    // Get and push the realm object onto the stack and set its metatable.
    realm_object_t** object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
    luaL_setmetatable(L, RealmHandle);
    *object = realm_results_get_object(*realm_results, index);

    return 1;
}

static int lib_realm_results_count(lua_State* L) {
    // Get argument from the stack.
    realm_results_t** realm_results = (realm_results_t**)lua_touserdata(L, 1);

    // Get the number of objects in the results/collection.
    size_t count;
    bool status = realm_results_count(*realm_results, &count);
    if (!status) {
        return _inform_realm_error(L);
    };

    // TODO?: size_t: typedef unsigned long size_t. (Change from lua_pushinteger?)
    lua_pushinteger(L, count);

    return 1;
}

static realm_query_t* lib_realm_query_parse(lua_State* L, realm_t *realm, const char* class_name, const char* query_string, size_t num_args, size_t lua_arg_offset) {
    // The start location of arguments on the stack.
    int arg_index;

    // Set up 2d vector to contain all realm_arguments provided.
    std::vector<realm_query_arg_t> args_vector;
    std::vector<std::vector<realm_value_t>> value;
    for (int index = 0; index < num_args; index++) {
        std::vector<realm_value_t>& realm_values = value.emplace_back();
        arg_index = lua_arg_offset + index;
        std::optional<realm_value_t> value = lua_to_realm_value(L, arg_index);
        if (!value) {
            // Unknown value provided.
            return nullptr;
        }
        realm_values.emplace_back(*value);
        args_vector.emplace_back(realm_query_arg_t {
            .nb_args = 1,
            .is_list = false,
            .arg = realm_values.data(),
        });
    }
    // Set array value here from vector.

    // Get class key.
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

static int lib_realm_results_filter(lua_State *L) {
    // Get the arguments from the stack.
    realm_results_t** unfiltered_result = (realm_results_t**)lua_touserdata(L, 1);
    realm_t** realm = (realm_t**)lua_touserdata(L, 2);
    const char* class_name = lua_tostring(L, 3);
    const char* query_string = lua_tostring(L, 4);
    size_t num_args = lua_tointeger(L, 5);

    // Parse the query string.
    size_t lua_arg_offset = 6;
    realm_query_t* query = lib_realm_query_parse(L, *realm, class_name, query_string, num_args, lua_arg_offset);
    if (!query) {
        return _inform_realm_error(L);
    }

    // Get and push the filtered result onto the stack and set its metatable.
    realm_results_t** result = static_cast<realm_results_t**>(lua_newuserdata(L, sizeof(realm_results_t*)));
    luaL_setmetatable(L, RealmHandle);
    *result = realm_results_filter(*unfiltered_result, query);

    return 1;
}


static int lib_realm_list_insert(lua_State *L) {
    // Get arguments from the stack.
    std::optional<realm_value_t> value = lua_to_realm_value(L, 3);
    if (!value) {
        _inform_error(L, "No corresponding realm value found");
        return 0;
    }
    realm_list_t** realm_list = (realm_list_t**)lua_touserdata(L, 1);
    size_t index = lua_tointeger(L, 2);

    // Get size of list.
    size_t out_size;
    if (!realm_list_size(*realm_list, &out_size)) {
        return _inform_realm_error(L);
    }

    // Append the value to the end of the list or at a specified index in between.
    bool success;
    if (index == out_size) {
        success = realm_list_insert(*realm_list, index, *value);
    }
    else if (index < out_size) {
        success = realm_list_set(*realm_list, index, *value);
    }
    else {
        return _inform_error(L, "Index out of bounds when setting value in list");
    }
    if (!success) {
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_list_get(lua_State *L) {
    // Get arguments from the stack.
    realm_list_t** realm_list = (realm_list_t**)lua_touserdata(L, 1);
    realm_t** realm = (realm_t**)lua_touserdata(L, 2);
    size_t index = lua_tointeger(L, 3);

    // Get value from list.
    realm_value_t out_value;
    if (!realm_list_get(*realm_list, index, &out_value)) {
        return _inform_realm_error(L);
    }

    // Convert the Realm value to its corresponding Lua value and push onto the stack.
    return realm_to_lua_value(L, *realm, out_value);
}

static int lib_realm_list_size(lua_State *L) {
    // Get arguments from the stack.
    realm_list_t** realm_list = (realm_list_t**)lua_touserdata(L, 1);

    // Get size of list and push onto the stack.
    size_t out_size;
    if (!realm_list_size(*realm_list, &out_size)) {
        return _inform_realm_error(L);
    }
    lua_pushinteger(L, out_size);

    return 1;
}

static int lib_realm_list_erase(lua_State *L)
{
    // Get arguments from the stack.
    realm_list_t** realm_list = (realm_list_t**)lua_touserdata(L, 1);
    size_t index = lua_tointeger(L, 2);

    if (!realm_list_erase(*realm_list, index))
    {
        return _inform_realm_error(L);
    }
    return 0;
}

static int lib_realm_get_list(lua_State *L) {
    // Get arguments from the stack.
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 1);
    realm_property_key_t& property_key = *(static_cast<realm_property_key_t*>(lua_touserdata(L, 2)));

    // Get and push the list onto the stack and set its metatable.
    realm_list_t** realm_list = static_cast<realm_list_t**>(lua_newuserdata(L, sizeof(realm_list_t*)));
    luaL_setmetatable(L, RealmHandle);
    *realm_list = realm_get_list(*realm_object, property_key);

    return 1;
}

static int lib_realm_get_set(lua_State *L)
{
    // Get arguments from the stack.
    realm_object_t **realm_object = (realm_object_t **)lua_touserdata(L, 1);
    realm_property_key_t &property_key = *(static_cast<realm_property_key_t *>(lua_touserdata(L, 2)));

    // Get and push the set onto the stack and set its metatable.
    realm_set_t **realm_set = static_cast<realm_set_t **>(lua_newuserdata(L, sizeof(realm_list_t *)));
    luaL_setmetatable(L, RealmHandle);
    *realm_set = realm_get_set(*realm_object, property_key);
    if (!*realm_set)
    {
        _inform_realm_error(L);
        return 0;
    }
    return 1;
}

static int lib_realm_set_insert(lua_State *L)
{
    std::optional<realm_value_t> value = lua_to_realm_value(L, 2);
    realm_set_t **realm_set = (realm_set_t **)lua_touserdata(L, 1);

    if (!realm_set_insert(*realm_set, *value, NULL, NULL))
    {
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_set_size(lua_State *L)
{
    // Get arguments from the stack.
    realm_set_t **realm_set = (realm_set_t **)lua_touserdata(L, 1);

    // Get size of list and push onto the stack.
    size_t out_size;
    if (!realm_set_size(*realm_set, &out_size))
    {
        return _inform_realm_error(L);
    }
    lua_pushinteger(L, out_size);

    return 1;
}

static int lib_realm_set_erase(lua_State *L)
{
    // Get arguments from the stack.
    realm_set_t **realm_set = (realm_set_t **)lua_touserdata(L, 1);
    std::optional<realm_value_t> value = lua_to_realm_value(L, 2);

    // Get size of list and push onto the stack.
    bool out_erased;
    if (!realm_set_erase(*realm_set, *value, &out_erased))
    {
        return _inform_realm_error(L);
    }
    lua_pushboolean(L, out_erased);
    return 1;
}

static int lib_realm_set_find(lua_State *L)
{
    // Get arguments from the stack.
    realm_set_t **realm_set = (realm_set_t **)lua_touserdata(L, 1);
    std::optional<realm_value_t> value = lua_to_realm_value(L, 2);
    bool out_found;
    if (!realm_set_find(*realm_set, *value, nullptr, &out_found))
    {
        return _inform_realm_error(L);
    }
    lua_pushboolean(L, out_found);

    return 1;
}

static const luaL_Reg lib[] = {
  {"realm_open",                                lib_realm_open},
  {"realm_release",                             lib_realm_release},
  {"realm_begin_write",                         lib_realm_begin_write},
  {"realm_commit_transaction",                  lib_realm_commit_transaction},
  {"realm_cancel_transaction",                  lib_realm_cancel_transaction},
  {"realm_object_create",                       lib_realm_object_create},
  {"realm_object_create_with_primary_key",      lib_realm_object_create_with_primary_key},
  {"realm_object_delete",                       lib_realm_object_delete},
  {"realm_set_value",                           lib_realm_set_value},
  {"realm_get_value",                           lib_realm_get_value},
  {"realm_object_is_valid",                     lib_realm_object_is_valid},
  {"realm_object_get_all",                      lib_realm_object_get_all},
  {"realm_object_add_listener",                 lib_realm_object_add_listener},
  {"realm_results_get",                         lib_realm_results_get},
  {"realm_results_count",                       lib_realm_results_count},
  {"realm_results_add_listener",                lib_realm_results_add_listener},
  {"realm_results_filter",                      lib_realm_results_filter},
  {"realm_list_insert",                         lib_realm_list_insert},
  {"realm_list_get",                            lib_realm_list_get},
  {"realm_list_size",                           lib_realm_list_size},
  {"realm_list_erase",                          lib_realm_list_erase},
  {"realm_get_list",                            lib_realm_get_list},
  {"realm_get_set",                             lib_realm_get_set},
  {"realm_set_size",                            lib_realm_set_size},
  {"realm_set_erase",                           lib_realm_set_erase},
  {"realm_set_find",                            lib_realm_set_find},
  {"realm_set_insert",                          lib_realm_set_insert},
  {NULL, NULL}
};


extern "C" int luaopen_realm_native(lua_State* L) {
    const luaL_Reg realm_handle_funcs[] = {
        {"__gc", lib_realm_release},
        {NULL, NULL}
    };
    luaL_newmetatable(L, RealmHandle);
    luaL_setfuncs(L, realm_handle_funcs, 0);
    lua_pop(L, 1); // Pop the RealmHandle metatable off the stack.

    luaL_newlib(L, lib);

    return 1;
}
