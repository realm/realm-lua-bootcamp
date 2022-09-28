#include <iostream>
#include <string>

#include <lua.hpp>

#include "../src/realm_native_lib.hpp"
#include "../src/realm_scheduler.hpp"
#include "../src/realm_app.hpp"
#include "../src/realm_user.hpp"

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
    if (status == LUA_OK)
        status = docall(L, 0, 0);

    return report(L, status);
}


static int dofile(lua_State *L, const char *name) {
    return dochunk(L, luaL_loadfile(L, name));
}


static int dostring(lua_State *L, const char *s, const char *name) {
    return dochunk(L, luaL_loadbuffer(L, s, strlen(s), name));
}

static void createargtable (lua_State *L, char **argv, int argc, int script) {
    int i, narg;
    narg = argc - (script + 1);  /* number of positive indices */
    lua_createtable(L, narg, script + 1);
    for (i = 0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - script);
    }
    lua_setglobal(L, "arg");
}

int main(int argc, char** argv) {
    std::string lua_path;
    if (const char* existing_path = getenv("LUA_PATH")) {
        lua_path = existing_path;
        lua_path += ";";
    }
    lua_path += SCRIPT_SOURCE_PATH "/lib/?/init.lua;";
    lua_path += SCRIPT_SOURCE_PATH "/lib/?.lua;";
    lua_path += ";";
    setenv("LUA_PATH", lua_path.c_str(), true);

    // Create a new instance of the Lua VM state object
    lua_State* L = luaL_newstate();

    // Load built-in libraries in the VM instance
    luaL_openlibs(L);
    luaL_requiref(L, "realm.native", luaopen_realm_native, 0);
    luaL_requiref(L, "realm.scheduler.libuv.native", luaopen_realm_scheduler_libuv_native, 0);
    luaL_requiref(L, "realm.app.native", luaopen_realm_app_native, 0);
    luaL_requiref(L, "realm.app.user.native", luaopen_realm_app_user_native, 0);

    const char* file = SCRIPT_SOURCE_PATH"/main.lua";

    if (argc > 1) {
        // arguments here are what the Lua vscode debugger passes to inject itself
        int fileArgIndex = 0;
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
                fileArgIndex = i;
                break;
            }
        }
        createargtable(L, argv, argc, fileArgIndex);
    }

    dofile(L, file);

    lua_close(L);

    return 0;
}
