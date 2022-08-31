#include <iostream>

#include <lua.hpp>

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
    return 0;
}