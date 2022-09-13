#include <vector>
#include <iostream>
#include <realm/util/to_string.hpp>
#include "realm_native_lib.hpp"
#include <realm.h>

static const char* RealmHandle = "_realm_handle";

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
static void _inform_error(lua_State* L, const char* format, Args&&... args) {
    lua_pushstring(L, realm::util::format(format, args...).c_str());
    lua_error(L);
}

static void _inform_realm_error(lua_State* L) {
    realm_error_t error;
    realm_get_last_error(&error);
    _inform_error(L, error.message);
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
        _inform_realm_error(L);
        return 0;
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
        _inform_realm_error(L);
        return 1;
    }

    return 1;
}

static int _lib_realm_release(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t** realm = static_cast<realm_t**>(lua_touserdata(L, -1));
    realm_release(*realm);
    return 0;
}

static int _lib_realm_begin_write(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_begin_write(*realm);
    if (!status){
        // Exception ocurred while trying to start a write transaction
        _inform_realm_error(L);
        return 0;
    }
    return 0;
}

static int _lib_realm_commit_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_commit(*realm);
    if (!status){
        // Exception ocurred while trying to commit a write transaction
        _inform_realm_error(L);
        return 0;
    }
    return 0;
}

static int _lib_realm_cancel_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_rollback(*realm);
    if (!status){
        // Exception ocurred while trying to cancel a write transaction
        _inform_realm_error(L);
        return 0;
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
        _inform_realm_error(L);
        lua_pop(L, 1);
        return 0;
    }
    if (!found) {
        _inform_error(L, "Class %1 not found", class_name);
        lua_pop(L, 1);
        return 0;
    }

    // Create object and feed it into the RealmObject handle
    *realm_object = realm_object_create(*realm, class_info.key); 
    if (!*realm_object) {
        // Exception ocurred when creating an object
        _inform_realm_error(L);
        lua_pop(L, 1);
        return 0;
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
        _inform_realm_error(L);
        return 0;
    }
    if (!found) {
        _inform_error(L, "Property %1 not found", property);
        return 0;
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
        _inform_error(L, "No valid type found");
        return 0;
    }

    if (!realm_set_value(*realm_object, property_info.key, value, false)) {
        // Exception ocurred when setting value
        _inform_realm_error(L);
        return 0;
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
        _inform_realm_error(L);
        return 0;
    }
    if (!found){
        _inform_error(L, "Unable to fetch value from property %1", property);
        return 0;
    }
    
    // Fetch desired value
    realm_value_t out_value;
    if (!realm_get_value(*realm_object, property_info.key, &out_value)) {
        // Exception ocurred while trying to fetch value
        _inform_realm_error(L);
        return 0;
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
        _inform_error(L, "Uknown type");
        return 0;
    }
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
  {"realm_open",                _lib_realm_open},
  {"realm_release",             _lib_realm_release},
  {"realm_begin_write",         _lib_realm_begin_write},
  {"realm_commit_transaction",  _lib_realm_commit_transaction},
  {"realm_cancel_transaction",  _lib_realm_cancel_transaction},
  {"realm_object_create",       _lib_realm_object_create},
  {"realm_set_value",           _lib_realm_set_value},
  {"realm_get_value",           _lib_realm_get_value},
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
