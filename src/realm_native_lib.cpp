#include <vector>
#include <iostream>
#include <realm/util/to_string.hpp>
#include "realm_native_lib.hpp"

static const char* RealmHandle = "_realm_handle";

struct realm_lua_userdata {
    lua_State* L;
    int callback_reference;
};

#define realm_userdata_t realm_lua_userdata*

// Needs to be included after realm_userdata_t
#include <realm.h>

// Checks whether given fullString ends with ending 
bool ends_with (const std::string_view& full_string, const std::string_view& ending) {
    if (full_string.length() >= ending.length()) {
        return (0 == full_string.compare (full_string.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

// Informs the user about the error through Lua API. 
// Supports a format string in form "%1...%2..."
template <typename... Args>
static int _inform_error(lua_State* L, const char* format, Args&&... args) {
    lua_pushstring(L, realm::util::format(format, args...).c_str());
    return lua_error(L);

    // TODO:
    // Compare to luaL_error
}

static int _inform_realm_error(lua_State* L) {
    realm_error_t error;
    realm_get_last_error(&error);
    return _inform_error(L, error.message);
}

static void _parse_property_type(lua_State* L, realm_property_info_t& prop, std::string_view type) {
    if (!type.size()) {
        _inform_error(L, "");
    }
    prop.flags = RLM_PROPERTY_NORMAL;
    
    if (ends_with(type, "[]")) {
        prop.collection_type = RLM_COLLECTION_TYPE_LIST;
        type = type.substr(0, type.size() - 2);
    }

    if (ends_with(type, "<>")) {
        prop.collection_type = RLM_COLLECTION_TYPE_SET;
        type = type.substr(0, type.size() - 2);
    }

    if (ends_with(type, "?")) {
        prop.flags |= RLM_PROPERTY_NULLABLE;
        type = type.substr(0, type.size() - 1);
    }

    if (ends_with(type, "{}")) {
        prop.collection_type = RLM_COLLECTION_TYPE_DICTIONARY;
        type = type.substr(0, type.size() - 2);

        if (type == "") {
            prop.type = RLM_PROPERTY_TYPE_MIXED; 
            prop.flags |= RLM_PROPERTY_NULLABLE;
            return;
        }
    }

    if (type == "bool") {
        prop.type = RLM_PROPERTY_TYPE_BOOL;
    }
    else if (type == "mixed") {
        prop.type = RLM_PROPERTY_TYPE_MIXED;
    }
    else if (type == "int") {
        prop.type = RLM_PROPERTY_TYPE_INT;
    }
    else if (type == "float") {
        prop.type = RLM_PROPERTY_TYPE_FLOAT;
    }
    else if (type == "double") {
        prop.type = RLM_PROPERTY_TYPE_DOUBLE;
    }
    else if (type == "string") {
        prop.type = RLM_PROPERTY_TYPE_STRING;
    }
    else if (type == "date") {
        prop.type = RLM_PROPERTY_TYPE_TIMESTAMP;
    }
    else if (type == "data") {
        prop.type = RLM_PROPERTY_TYPE_BINARY;
    }
    else if (type == "decimal128") {
        prop.type = RLM_PROPERTY_TYPE_DECIMAL128;
    }
    else if (type == "objectId") {
        prop.type = RLM_PROPERTY_TYPE_OBJECT_ID;
    }
    else if (type == "uuid") {
        prop.type = RLM_PROPERTY_TYPE_UUID;
    }
    else {
        _inform_error(L, "Unknown type");
        prop.type = RLM_PROPERTY_TYPE_MIXED;
    }
}

static realm_schema_t* _parse_schema(lua_State* L) {
    size_t classes_len = lua_rawlen(L, -1);
    
    // Array of classes and a two-dimensional
    // array of properties for every class.
    realm_class_info_t classes[classes_len];
    memset(classes, 0, sizeof(realm_class_info_t)*classes_len);
    
    const realm_property_info_t* properties[classes_len];
    // 2D vector of class properties to act as a memory buffer
    // for the actual properties array.
    std::vector<std::vector<realm_property_info_t>> properties_vector = {};

    int argument_index = lua_gettop(L);
    for (size_t i = 1; i <= classes_len; i++) {
        lua_rawgeti(L, argument_index, i);

        // Use name field to create initial class info
        lua_getfield(L, -1, "name");
        realm_class_info_t& class_info = classes[i-1];
        class_info.name = lua_tostring(L, -1);
        class_info.primary_key = "";
        lua_pop(L, 1);

        // Get properties and iterate through them
        lua_getfield(L, -1, "properties");
        luaL_checktype(L, -1, LUA_TTABLE);

        // Iterate through key-values of a specific class' properties table.
        // (Push nil since lua_next starts by popping.)
        std::vector<realm_property_info_t>& class_properties = *properties_vector.emplace({});
        lua_pushnil(L);
        while(lua_next(L, -2) != 0) {
            // Copy the key.
            lua_pushvalue(L, -2);
            // The copied key.
            luaL_checktype(L, -1, LUA_TSTRING);
            // The value.
            luaL_checktype(L, -2, LUA_TSTRING);

            realm_property_info_t& property_info = class_properties.emplace_back(realm_property_info_t{
                .name = lua_tostring(L, -1),
                // TODO?: add support for this
                .public_name = "",
                .link_target = "",
                .link_origin_property_name = "",
            });
            _parse_property_type(L, property_info, lua_tostring(L, -2));
            lua_pop(L, 2);
        }
        // Drop the properties field.
        lua_pop(L, 1);
         
        // Add the parsed class and property information to the array.
        class_info.num_properties = class_properties.size();        
        properties[i-1] = class_properties.data();
    }
    // Pop the last class index.
    lua_pop(L, 1);

    return realm_schema_new(classes, classes_len, properties);
}

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
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_begin_write(*realm);
    if (!status){
        // Exception ocurred while trying to start a write transaction
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_commit_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_commit(*realm);
    if (!status){
        // Exception ocurred while trying to commit a write transaction
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_cancel_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
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
    realm_t **realm = (realm_t **)lua_touserdata(L, 1);
    const char *class_name = lua_tostring(L, 2);

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
    realm_t **realm = (realm_t **)lua_touserdata(L, 1);
    realm_object_t **realm_object = (realm_object_t **)lua_touserdata(L, 2);
    const char *property = lua_tostring(L, 3);

    // Get the property to update based on its string representation
    realm_property_info_t property_info;
    realm_class_key_t class_key = realm_object_get_table(*realm_object);
    bool found = false;
    if (!realm_find_property(*realm, class_key, property, &found, &property_info)) {
        // Exception ocurred when fetching the property 
        return _inform_realm_error(L);
    }
    if (!found) {
        return _inform_error(L, "Property %1 not found", property);
    }

    // Translate the lua value into corresponding realm value
    realm_value_t value;
    switch (property_info.type)
    {
    case RLM_PROPERTY_TYPE_INT:
        value.type = RLM_TYPE_INT;
        value.integer = lua_tonumber(L, 4);
        break;
    case RLM_PROPERTY_TYPE_BOOL:
        value.type = RLM_TYPE_BOOL;
        value.boolean = lua_toboolean(L, 4);
        break;
    case RLM_PROPERTY_TYPE_STRING:
        value.type = RLM_TYPE_STRING;
        value.string.size = lua_rawlen(L, 4);
        value.string.data = lua_tostring(L, 4);
        break;
    case RLM_PROPERTY_TYPE_DOUBLE:
        value.type = RLM_TYPE_DOUBLE;
        value.dnum = lua_tonumber(L, 4);
        break;
    default:
        return _inform_error(L, "No valid type found");
    }

    if (!realm_set_value(*realm_object, property_info.key, value, false)) {
        // Exception ocurred when setting value
        return _inform_realm_error(L);
    }
    return 0;
}

static int _lib_realm_get_value(lua_State* L) {
    // Get arguments from stack
    realm_t **realm = (realm_t **)lua_touserdata(L, 1);
    realm_object_t **realm_object = (realm_object_t **)lua_touserdata(L, 2);
    const char *property = lua_tostring(L, 3);

    // Get the property to fetch from based on its string representation
    realm_property_info_t property_info;
    realm_class_key_t class_key = realm_object_get_table(*realm_object);
    bool found = false;
    if (!realm_find_property(*realm, class_key, property, &found, &property_info)) {
        // Exception ocurred when fetching the property 
        return _inform_realm_error(L);
    }
    if (!found){
        return _inform_error(L, "Unable to fetch value from property %1", property);
    }
    
    // Fetch desired value
    realm_value_t out_value;
    if (!realm_get_value(*realm_object, property_info.key, &out_value)) {
        // Exception ocurred while trying to fetch value
        return _inform_realm_error(L);
    }

    // Push correct lua value based on Realm type
    switch (out_value.type)
    {
    case RLM_TYPE_NULL:
        lua_pushnil(L);
        break;
    case RLM_TYPE_INT:
        lua_pushinteger(L, out_value.integer);
        break;
    case RLM_TYPE_BOOL:
        lua_pushboolean(L, out_value.boolean);
        break;
    case RLM_TYPE_STRING:
        lua_pushstring(L, out_value.string.data);
        break;
    case RLM_TYPE_FLOAT:
        lua_pushnumber(L, out_value.fnum);
        break;
    case RLM_TYPE_DOUBLE:
        lua_pushnumber(L, out_value.dnum);
        break;
    default:
        return _inform_error(L, "Uknown type");
    }
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

        // NOTE:
        // - there are two available functions in the C API: realm_find_class and realm_get_class
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

void on_collection_change(realm_lua_userdata* userdata, const realm_collection_changes_t* changes) {
    lua_State* L = userdata->L;
    int callback_reference = userdata->callback_reference;

    // Put the Lua listener function on the top of the stack.
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback_reference);

    // TODO: Go through the change set and build arguments.
    // (Lua expects: 1st arg: collection, 2nd arg: changes)
    lua_pushstring(L, "Hello");
    lua_pushstring(L, "World");
    int status = lua_pcall(L, 2, 0, 0);
    if (status != LUA_OK) {
        _inform_error(L, "Could not call the listener function.");
        return;
    }
}

void free_userdata(realm_lua_userdata* userdata) {
    // Get the callback reference from the registry and unreference it
    // so that Lua can garbage collect it.
    luaL_unref(userdata->L, LUA_REGISTRYINDEX, userdata->callback_reference);
    delete userdata;
}

static int _lib_realm_results_add_listener(lua_State* L) {
    // Get 1st argument (results/collection) from stack
    realm_results_t **results = (realm_results_t**)lua_touserdata(L, 1);
    
    // Pop 2nd argument/top of stack (the Lua function) from the stack and save a
    // refernce to it in the register. "callback_reference" is the register location.
    int callback_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    // Create a pointer to userdata for use in the callback that
    // will be invoked at a later time.
    realm_lua_userdata* userdata = new realm_lua_userdata;
    userdata->L = L;
    userdata->callback_reference = callback_reference;

    // TODO:
    // Deal with notification_token

    // Put the notification token on the stack.
    auto** notification_token = static_cast<realm_notification_token_t**>(lua_newuserdata(L, sizeof(realm_notification_token_t*)));
    *notification_token = realm_results_add_notification_callback(
        *results,
        userdata,
        free_userdata,
        nullptr,
        on_collection_change
    );

    // Set the metatable of the notification token to that of RealmHandle
    // in order for it to be released via __gc.
    luaL_setmetatable(L, RealmHandle);

    if (!*notification_token) {
        lua_pop(L, 1);
        return _inform_realm_error(L);
    }

    return 1;
}

// TODO
static int _lib_realm_object_add_listener(lua_State* L) {

    // realm.h
    // RLM_API realm_notification_token_t* realm_object_add_notification_callback(
    //     realm_object_t*,
    //     realm_userdata_t userdata,
    //     realm_free_userdata_func_t userdata_free,
    //     realm_key_path_array_t*,
    //     realm_on_object_change_func_t on_change
    // );

    return 0;
}

static realm_query_t* _lib_realm_query_parse(lua_State* L, realm_t *realm, const char* class_name, const char* query_string, size_t num_args, size_t lua_arg_offset) {
    // Value which keeps track of the start location of arguments on the stack
    size_t arg_index;

    // Setup 2d vector to contain all realm_arguments provided
    std::vector<realm_query_arg_t> args_vector;
    std::vector<std::vector<realm_value_t>> value;
    for(int index = 0; index < num_args; index++){
        std::vector<realm_value_t>& realm_values = value.emplace_back();
        arg_index = lua_arg_offset + index;
        // TODO: add support for lists as input
        if (lua_type(L, arg_index) == LUA_TNUMBER){
            // Value is either of type double or int, need further investigation
            if (lua_isinteger(L, arg_index)){
                realm_values.emplace_back(realm_value_t{
                    .type = RLM_TYPE_INT,
                    .integer = lua_tointeger(L, arg_index),
                });
            } else {
                realm_values.emplace_back(realm_value_t{
                    .type = RLM_TYPE_DOUBLE,
                    .dnum = lua_tonumber(L, arg_index),
                });
            }
        } else if (lua_type(L, arg_index) == LUA_TSTRING){
            realm_values.emplace_back(realm_value_t{
                .type = RLM_TYPE_STRING,
                .string.size = lua_rawlen(L, arg_index),
                .string.data = lua_tostring(L, arg_index),
            });
        } else if (lua_type(L, arg_index) == LUA_TBOOLEAN){
            realm_values.emplace_back(realm_value_t{
                .type = RLM_TYPE_BOOL,
                .boolean = static_cast<bool>(lua_toboolean(L, arg_index)),
            });
        }
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

// static int _lib_realm_tostring(lua_State* L) {
//     luaL_checkudata(L, 1, RealmHandle);
//     void** value = static_cast<void**>(lua_touserdata(L, -1));
//     realm_release(*value);
//     // RLM_API bool realm_get_value(const realm_object_t*, realm_property_key_t, realm_value_t* out_value);
//     return 0;
// }

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
  {"realm_results_filter",       _lib_realm_results_filter},
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
    // lua_pushstring(L, "__tostring");
    // lua_pushcfunction(L, &_lib_realm_tostring);
    // lua_settable(L, -3);
}
