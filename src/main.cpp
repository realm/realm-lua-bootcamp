#include <iostream>
#include <string>

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

static int msghandler(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) {  /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
            return 1;  /* that is the message */
    else
        msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
    return 1;  /* return the traceback */
}

static int report(lua_State *L, int status) {
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        lua_writestringerror("%s\n", msg);
        lua_pop(L, 1);  /* remove message */
    }
    return status;
}

static int docall(lua_State *L, int narg, int nres) {
    int status;
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, msghandler);  /* push message handler */
    lua_insert(L, base);  /* put it under function and args */
    status = lua_pcall(L, narg, nres, base);
    lua_remove(L, base);  /* remove message handler from the stack */
    return status;
}

static int dochunk(lua_State *L, int status) {
    if (status == LUA_OK) status = docall(L, 0, 0);
    return report(L, status);
}


static int dofile(lua_State *L, const char *name) {
    return dochunk(L, luaL_loadfile(L, name));
}


static int dostring(lua_State *L, const char *s, const char *name) {
    return dochunk(L, luaL_loadbuffer(L, s, strlen(s), name));
}

int main(int argc, char** argv) {
    std::string lua_path;
    if (const char* existing_path = getenv("LUA_PATH")) {
        lua_path = existing_path;
        lua_path += ";";
    }
    lua_path += SCRIPT_SOURCE_PATH "/lib/?/init.lua;;";
    setenv("LUA_PATH", lua_path.c_str(), true);

    // Create a new instance of the Lua VM state object
    lua_State* L = luaL_newstate();

    // Load built-in libraries in the VM instance
    luaL_openlibs(L);
    realm_lib_open(L);

    const char* file = SCRIPT_SOURCE_PATH"/main.lua";

    if (argc > 1) {
        // arguments here are what the Lua vscode debugger passes to inject itself
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                switch (argv[i][1]) {
                    case 'e':
                        dostring(L, argv[++i], "=(command line)");
                        break;
                    default:
                        assert(false);
                }
            } else {
                file = argv[i];
            }
        }
    }

    dofile(L, file);
    lua_close(L);
    return 0;
}