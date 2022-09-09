#include <vector>
#include <iostream>

#include "realm_native_lib.hpp"
#include <realm.h>

static const char* RealmHandle = "_realm_handle";

static realm_property_type_e _parse_property_type(const char* propstring) {
    if (strcmp(propstring, "int") == 0) return RLM_PROPERTY_TYPE_INT;
    else if (strcmp(propstring, "string") == 0) return RLM_PROPERTY_TYPE_STRING;
    else {
        // TODO: all types
        std::cerr << "Unknown type " << propstring << "\n";
        return RLM_PROPERTY_TYPE_MIXED;
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

            class_properties.push_back(realm_property_info_t{
                // -1 is equivalent to the table key.
                .name = lua_tostring(L, -1),
                // TODO?: add support for this
                .public_name = "",
                // -2 is equivalent to the table value
                .type = _parse_property_type(lua_tostring(L, -2)),

                .link_target = "",
                .link_origin_property_name = "",
            });
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

    realm_error_t error;
    
    lua_getfield(L, 1, "schema");
    realm_schema_t* schema = _parse_schema(L);
    if (!schema) {
        realm_get_last_error(&error);
        // TODO: print error
        return 1;
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
        realm_get_last_error(&error);
        // TODO: print error
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
        std::cerr << "Unable to start transaction" << std::endl;
    }
    return 0;
}

static int _lib_realm_commit_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_commit(*realm);
    if (!status){
        std::cerr << "Unable to commit transaction" << std::endl;
    }
    return 0;
}

static int _lib_realm_cancel_transaction(lua_State* L) {
    luaL_checkudata(L, 1, RealmHandle);
    realm_t **realm = (realm_t **)lua_touserdata(L, -1);
    bool status = realm_rollback(*realm);
    if (!status){
        std::cerr << "Unable to rollback transaction" << std::endl;
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

    // realm_error_t error; // TODO: Can we use this for error handling?
    // Get class key corresponding to the object we create
    realm_class_info_t class_info;
    bool found = false;
    if (!realm_find_class(*realm, class_name, &found, &class_info)) {
        std::cerr << "did not find class" << std::endl;
        lua_pop(L, 1);
        return 0;
    }

    // Create object and feed it into the RealmObject handle
    *realm_object = realm_object_create(*realm, class_info.key); 
    if (!*realm_object) {
        std::cerr << "could not create object" << std::endl;
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
    bool out_found = false;
    realm_find_property(*realm, class_key, property, &out_found, &property_info);

    // Translate the lua value into corresponding realm value
    realm_value_t value;
    switch (lua_type(L,4))
    {
    case LUA_TNUMBER:
        value.type = RLM_TYPE_INT;
        value.integer = lua_tointeger(L, 4);
        break;
    case LUA_TSTRING:
        value.type = RLM_TYPE_STRING;
        value.string.size = lua_rawlen(L, 4);
        value.string.data = lua_tostring(L, 4);
        break;
    default:
        break;
    }

    realm_error_t error;
    if (!realm_set_value(*realm_object, property_info.key, value, false)) {
        realm_get_last_error(&error);
        std::cerr << "Unable to update value" << std::endl;
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
    bool out_found = false;
    realm_find_property(*realm, class_key, property, &out_found, &property_info);
    if (!out_found){
        std::cerr << "Unable to find property" << std::endl;
    }
    
    // Fetch desired value
    realm_value_t out_value;
    if (!realm_get_value(*realm_object, property_info.key, &out_value)) {
        std::cerr << "Unable to get value" << std::endl; 
        return 0;
    }

    // Push correct lua value based on Realm type
    switch (out_value.type)
    {
    case RLM_TYPE_INT:
        lua_pushnumber(L, out_value.integer);
        break;
    case RLM_TYPE_STRING:
        lua_pushstring(L, out_value.string.data);
        break;
    default:
        std::cout << "uknown type\n";
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
  {"realm_open",        _lib_realm_open},
  {"realm_release",     _lib_realm_release},
  {"realm_begin_write", _lib_realm_begin_write},
  {"realm_commit_transaction", _lib_realm_commit_transaction},
  {"realm_cancel_transaction", _lib_realm_cancel_transaction},
  {"realm_object_create",        _lib_realm_object_create},
  {"realm_set_value",        _lib_realm_set_value},
  {"realm_get_value",        _lib_realm_get_value},
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
