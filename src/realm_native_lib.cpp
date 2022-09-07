#include "realm_native_lib.hpp"
#include <realm.h>

static const char* RealmReleaseMetatableName = "_realm_release";

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
    const realm_property_info_t* properties[classes_len];;

    int argument_index = lua_gettop(L);
    for (size_t i = 1; i <= classes_len; i++) {
        lua_rawgeti(L, argument_index, i);

        // Use name field to create initial class info
        lua_getfield(L, -1, "name");
        realm_class_info_t class_Info{
            .name = lua_tostring(L, -1),
            .primary_key = "",
            .num_properties = 0,
        };
        lua_pop(L, 1);

        // Get properties and iterate through them
        lua_getfield(L, -1, "properties");
        luaL_checktype(L, -1, LUA_TTABLE);

        std::vector<realm_property_info_t> class_properties = {};
    
        // Iterate through key-values of a specific class' properties table.
        // (Push nil since lua_next starts by popping.)
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
        class_Info.num_properties = class_properties.size();        
        properties[i-1] = class_properties.data();
        classes[i-1] = class_Info;
    }
    // Pop the last class index.
    lua_pop(L, 1);

    return realm_schema_new(classes, classes_len, properties);
}

static int _lib_realm_open(lua_State* L) {
    realm_t** realm = static_cast<realm_t**>(lua_newuserdata(L, sizeof(realm_t*)));
    luaL_setmetatable(L, RealmReleaseMetatableName);
    
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
    if (!realm) {
        realm_get_last_error(&error);
        // TODO: print error
        return 1;
    }

    return 1;
}

static int _lib_realm_begin_write(lua_State* L) {
    return 0;
}

static int _lib_realm_release(lua_State* L) {
    luaL_checkudata(L, 1, RealmReleaseMetatableName);

    void** value = static_cast<void**>(lua_touserdata(L, 1));
    realm_release(*value);

    return 0;
}

static const luaL_Reg lib[] = {
  {"realm_open",        _lib_realm_open},
  {"realm_begin_write", _lib_realm_begin_write},
  {"realm_release",     _lib_realm_release},
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

    luaL_newmetatable(L, RealmReleaseMetatableName);
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, &_lib_realm_release);
    lua_settable(L, -3);
}
