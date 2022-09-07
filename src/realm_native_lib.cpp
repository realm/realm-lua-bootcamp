#include "realm_native_lib.hpp"

#include <realm.h>

static const char* RealmReleaseMetatableName = "_realm_release";

static int _lib_realm_open(lua_State* L) {
    realm_t** realm = static_cast<realm_t**>(lua_newuserdata(L, sizeof(realm_t*)));
    luaL_setmetatable(L, RealmReleaseMetatableName);
    // *realm = realm_open(...)
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
