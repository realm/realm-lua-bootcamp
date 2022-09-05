#include <iostream>
#include <filesystem>

#include <realm.h>
#include <lua.hpp>

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

static int my_native_lua_sum(lua_State* L) {
    int number_of_arguments = lua_gettop(L);
    
    int64_t sum = 0;
    for (int arg = 1; arg <= number_of_arguments; arg++) {
        if (!lua_isnumber(L, arg)) {
            return luaL_error(L, "Argument number %d is not of type number!", arg);
        }
        sum += lua_tonumber(L, arg);
    }

    lua_pushinteger(L, sum);
    return 1; // number of returned values;
}

int main(int argc, char** argv) {
    // Create a new instance of the Lua VM state object
    lua_State* L = luaL_newstate();

    // Load built-in libraries in the VM instance
    luaL_openlibs(L);

    // Push native function to the stack, and expose it as a global
    lua_pushcfunction(L, &my_native_lua_sum);
    lua_setglobal(L, "my_custom_sum");

    int status;

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