#include <vector>
#include <iostream>
#include <realm/util/to_string.hpp>

// Note: make sure to include realm_notifications before realm.h
#include "realm_notifications.hpp"
#include <realm.h>
#include "realm_native_lib.hpp"
#include "realm_schema.hpp"
#include "realm_util.hpp"

static int _lib_realm_open(lua_State* L) {
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
    lua_pop(L, 1);

    lua_getfield(L, 1, "schemaVersion");
    luaL_checkinteger(L, -1);
    realm_config_set_schema_version(config, lua_tointeger(L, -1));
    // TODO?: add ability to change this through config object? 
    realm_config_set_schema_mode(config, RLM_SCHEMA_MODE_SOFT_RESET_FILE); // delete realm file if there are schema conflicts
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "_cached");
    if (lua_isboolean(L, -1)) {
        realm_config_set_cached(config, lua_toboolean(L, -1));
    }
    lua_pop(L, 1);

    if (realm_scheduler_t** scheduler = static_cast<realm_scheduler_t**>(luaL_checkudata(L, 2, RealmHandle))) {
        realm_config_set_scheduler(config, *scheduler);
    }

    const realm_t** realm = static_cast<const realm_t**>(lua_newuserdata(L, sizeof(realm_t*)));
    luaL_setmetatable(L, RealmHandle);
    *realm = realm_open(config);
    realm_release(config);
    if (!*realm) {
        // Exception ocurred while trying to open realm
        return _inform_realm_error(L);
    }
    _push_schema_info(L, *realm);
    return 2;
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
    const int64_t class_key = lua_tointeger(L, 2);

    // Create object and feed it into the RealmObject handle
    *realm_object = realm_object_create(*realm, class_key); 
    if (!*realm_object) {
        // Exception ocurred when creating an object
        return _inform_realm_error(L);
    }
    return 1;
}

static int _lib_realm_object_create_with_primary_key(lua_State* L) {
    realm_object_t** realm_object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
    luaL_setmetatable(L, RealmHandle);
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    const int class_key = lua_tointeger(L, 2);
    std::optional<realm_value_t> pk = lua_to_realm_value(L, 3);
    if (!pk){
        // No corresponding realm value found
        return 0;
    }
    *realm_object = realm_object_create_with_primary_key(*realm, class_key, *pk); 
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
    realm_property_key_t& property_key = *(static_cast<realm_property_key_t*>(lua_touserdata(L, 3)));

    // Translate the lua value into corresponding realm value
    std::optional<realm_value_t> value = lua_to_realm_value(L, 4);
    if (!value){
        // No corresponding realm value found
        return 0;
    }

    if (!realm_set_value(*realm_object, property_key, *value, false)) {
        // Exception ocurred when setting value
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_get_value(lua_State* L) {
    // Get arguments from stack
    realm_t** realm = (realm_t**)lua_touserdata(L, 1);
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 2);
    realm_property_key_t& property_key = *(static_cast<realm_property_key_t*>(lua_touserdata(L, 3)));

    realm_value_t out_value;
    if (!realm_get_value(*realm_object, property_key, &out_value)) {
        // Exception ocurred while trying to fetch value
        return _inform_realm_error(L);
    }

    // Push correct lua value based on Realm type
    return realm_to_lua_value(L, *realm, out_value);
}

static int _lib_realm_object_delete(lua_State* L) {
    realm_object_t **object = (realm_object_t**)lua_touserdata(L, 1);
    if (realm_object_delete(*object)){
        return 0;
    } else {
        return _inform_realm_error(L);
    }
}

static int _lib_realm_object_is_valid(lua_State* L) {
    realm_object_t **object = (realm_object_t**)lua_touserdata(L, 1);
    lua_pushboolean(L, realm_object_is_valid(*object));
    return 1; 
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

    // Push results onto stack
    realm_results_t **results = static_cast<realm_results_t**>(lua_newuserdata(L, sizeof(realm_results_t*)));
    *results = realm_object_find_all(*realm, class_info.key);

    // Set the metatable of the results (top of stack) to that
    // of RealmHandle in order for it to be released via __gc.
    luaL_setmetatable(L, RealmHandle);

    return 1;
}

static int _lib_realm_results_get(lua_State* L) {
    // Get arguments from stack
    realm_results_t **realm_results = (realm_results_t **)lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);

    // Push realm object onto stack
    realm_object_t **object = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
    *object = realm_results_get_object(*realm_results, index);

    // Set the metatable of the object (top of stack) to that
    // of RealmHandle in order for it to be released via __gc.
    luaL_setmetatable(L, RealmHandle);

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

    // Set the metatable of the results (top of stack) to that
    // of RealmHandle in order for it to be released via __gc.
    luaL_setmetatable(L, RealmHandle);

    return 1;
}


static int _lib_realm_list_insert(lua_State *L){
    std::optional<realm_value_t> value = lua_to_realm_value(L, 3);
    if (!value){
        _inform_error(L, "No corresponding realm value found");
        return 0;
    }

    // If index is larger than size of list we invoke insert to append on the last place, otherwise set is called
    // Get values from lua stack
    realm_list_t **realm_list = (realm_list_t**)lua_touserdata(L, 1);
    size_t index = lua_tointeger(L, 2);

    // Get size of list 
    size_t out_size;
    if (!realm_list_size(*realm_list, &out_size)){
        return _inform_realm_error(L);
    }

    // Call correct insert function
    bool success;
    if (index == out_size){
        success = realm_list_insert(*realm_list, index, *value);
    } else if(index < out_size){
        success = realm_list_set(*realm_list, index, *value);
    } else {
        return _inform_error(L, "Index out of bounds when setting value in list");
    }
    if (!success){
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_list_get(lua_State *L){
    // Get values from lua stack
    realm_list_t** realm_list = (realm_list_t**)lua_touserdata(L, 1);
    realm_t** realm = (realm_t**)lua_touserdata(L, 2);
    size_t index = lua_tointeger(L, 3);

    // Get value from list 
    realm_value_t out_value;
    if (!realm_list_get(*realm_list, index, &out_value)){
        return _inform_realm_error(L);
    }

    // return corresponding lua value
    return realm_to_lua_value(L, *realm, out_value);
}

static int _lib_realm_list_size(lua_State *L){
    // Get values from lua stack
    realm_list_t** realm_list = (realm_list_t**)lua_touserdata(L, 1);

    // Get size of list 
    size_t out_size;
    if (!realm_list_size(*realm_list, &out_size)){
        return _inform_realm_error(L);
    }

    // Put size on lua stack
    lua_pushinteger(L, out_size);
    return 1;
}

static int _lib_realm_get_list(lua_State *L){
    realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, 1);
    realm_property_key_t& property_key = *(static_cast<realm_property_key_t*>(lua_touserdata(L, 2)));
    realm_list_t** realm_list = static_cast<realm_list_t**>(lua_newuserdata(L, sizeof(realm_list_t*)));
    *realm_list = realm_get_list(*realm_object, property_key);
    luaL_setmetatable(L, RealmHandle);
    return 1;
}

static const luaL_Reg lib[] = {
  {"realm_open",                            _lib_realm_open},
  {"realm_release",                         _lib_realm_release},
  {"realm_begin_write",                     _lib_realm_begin_write},
  {"realm_commit_transaction",              _lib_realm_commit_transaction},
  {"realm_cancel_transaction",              _lib_realm_cancel_transaction},
  {"realm_object_create",                   _lib_realm_object_create},
  {"realm_object_create_with_primary_key",  _lib_realm_object_create_with_primary_key},
  {"realm_object_delete",                   _lib_realm_object_delete},
  {"realm_object_is_valid",                 _lib_realm_object_is_valid},
  {"realm_set_value",                       _lib_realm_set_value},
  {"realm_get_value",                       _lib_realm_get_value},
  {"realm_object_get_all",                  _lib_realm_object_get_all},
  {"realm_object_add_listener",             _lib_realm_object_add_listener},
  {"realm_results_get",                     _lib_realm_results_get},
  {"realm_results_count",                   _lib_realm_results_count},
  {"realm_results_add_listener",            _lib_realm_results_add_listener},
  {"realm_results_filter",                  _lib_realm_results_filter},
  {"realm_list_insert",                     _lib_realm_list_insert},
  {"realm_list_get",                        _lib_realm_list_get},
  {"realm_list_size",                       _lib_realm_list_size},
  {"realm_get_list",                       _lib_realm_get_list},
  {NULL, NULL}
};

extern "C" int luaopen_realm_native(lua_State* L) {
    const luaL_Reg realm_handle_funcs[] = {
        {"__gc", _lib_realm_release},
        {NULL, NULL}
    };
    luaL_newmetatable(L, RealmHandle);
    luaL_setfuncs(L, realm_handle_funcs, 0);
    lua_pop(L, 1); // pop the RealmHandle metatable off the stack

    luaL_newlib(L, lib);
    return 1;
}
