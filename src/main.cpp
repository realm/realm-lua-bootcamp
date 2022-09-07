#include <iostream>
#include <filesystem>
#include <vector>

#include <realm.h>
#include <lua.hpp>

#include "realm_native_lib.hpp"

static void realm_first_steps() {
    realm_property_info_t person_name {
        .name = "name",
        .public_name = "",
        .type = RLM_PROPERTY_TYPE_STRING,

        .link_target = "",
        .link_origin_property_name = "",
    };
    realm_property_info_t person_age {
        .name = "age",
        .public_name = "",
        .type = RLM_PROPERTY_TYPE_INT,

        .link_target = "",
        .link_origin_property_name = "",
    };
    const realm_property_info_t person_properties[] {
        person_name,
        person_age
    };

    realm_class_info_t person_class {
        .name = "Person",
        .primary_key = "",
        .num_properties = sizeof(person_properties) / sizeof(realm_property_info_t),
    };

    realm_class_info_t classes[] {
        person_class,
    };

    const realm_property_info_t* properties[] {
        person_properties
    };

    realm_error_t error;

    realm_schema_t* schema = realm_schema_new(classes, sizeof(classes) / sizeof(realm_class_info_t), properties);
    if (!schema) {
        realm_get_last_error(&error);
        // TODO: print error
        return;
    }

    realm_config_t* config = realm_config_new();
    realm_config_set_path(config, "./bootcamp.realm");
    realm_config_set_schema(config, schema);
    realm_config_set_schema_version(config, 0);
    realm_config_set_schema_mode(config, RLM_SCHEMA_MODE_SOFT_RESET_FILE); // delete realm file if there are schema conflicts
    realm_release(schema);

    realm_t* realm = realm_open(config);
    realm_release(config);

    if (!realm) {
        realm_get_last_error(&error);
        // TODO: print error
        return;
    }

    bool found = false;
    if (!realm_find_class(realm, person_class.name, &found, &person_class)) {
        realm_get_last_error(&error);
        // TODO: print error
        return;
    }

    if (!realm_find_property(realm, person_class.key, person_name.name, &found, &person_name)) {
        realm_get_last_error(&error);
        // TODO: print error
        return;
    }

    realm_begin_write(realm);

    realm_object_t* person = realm_object_create(realm, person_class.key);
    if (!person) {
        realm_get_last_error(&error);
        // TODO: print error
        return;
    }

    realm_value_t name_value;
    name_value.type = RLM_TYPE_STRING;
    name_value.string = { "Mads", 4 };
    if (!realm_set_value(person, person_name.key, name_value, false)) {
        realm_get_last_error(&error);
        // TODO: print error
        return;
    }

    realm_commit(realm);

    realm_release(realm);
}

static realm_property_type_e property_type_from_string(const char* propstring) {
    if (strcmp(propstring, "integer") == 0) return RLM_PROPERTY_TYPE_INT;
    else if (strcmp(propstring, "string") == 0) return RLM_PROPERTY_TYPE_STRING;
    else {
        // TODO: all types
        std::cerr << "Unknown type " << propstring;
        return RLM_PROPERTY_TYPE_MIXED;
    }
}

static int l_realm_open(lua_State* L) {
    luaL_checktype(L, -1, LUA_TTABLE);

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
                .type = property_type_from_string(lua_tostring(L, -2)),

                .link_target = "",
                .link_origin_property_name = "",
            });
            lua_pop(L, 2);
        }

        // Add the parsed class and property information to the array.
        class_Info.num_properties = class_properties.size();        
        properties[i-1] = class_properties.data();
        classes[i-1] = class_Info;
    }
    realm_error_t error;

    realm_schema_t* schema = realm_schema_new(classes, classes_len, properties);
    if (!schema) {
        realm_get_last_error(&error);
        // TODO: print error
        return 1;
    }

    realm_config_t* config = realm_config_new();
    realm_config_set_path(config, "./bootcamp.realm");
    realm_config_set_schema(config, schema);
    realm_config_set_schema_version(config, 0);
    realm_config_set_schema_mode(config, RLM_SCHEMA_MODE_SOFT_RESET_FILE); // delete realm file if there are schema conflicts
    realm_release(schema);
    
    realm_t* realm = realm_open(config);
    realm_release(config);

    if (!realm) {
        realm_get_last_error(&error);
        // TODO: print error
        return 1;
    }
    realm_release(realm);
    return 1;
}

int main(int argc, char** argv) {
    // Create a new instance of the Lua VM state object
    lua_State* L = luaL_newstate();

    // Load built-in libraries in the VM instance
    luaL_openlibs(L);
    realm_lib_open(L);

    // Push native function to the stack, and expose it as a global
    lua_pushcfunction(L, &l_realm_open);
    lua_setglobal(L, "c_realm_open");

    int status;

    status = luaL_dostring(L, "package.path = package.path .. ';" SCRIPT_SOURCE_PATH "/lib/?/init.lua'");
    if (status) {
        std::cerr << "Error setting up package search path: " << lua_tostring(L, -1) << std::endl;
        return 1;
    }

    // Load script file and push it onto the stack
    status = luaL_loadfile(L, SCRIPT_SOURCE_PATH"/main.lua");
    if (status) {
        std::cerr << "Error loading file: " << lua_tostring(L, -1) << std::endl;
        return 1;
    }

    // Execute the value at top of the stack
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status) {
        std::cerr << "Error running script: " << lua_tostring(L, -1) << std::endl;
        return 1;
    }

    lua_close(L);

    realm_first_steps();

    return 0;
}