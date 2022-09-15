#include <lua.hpp>

#include <realm/util/to_string.hpp>

static const char* RealmHandle = "_realm_handle";

template <typename... Args>
int _inform_error(lua_State* L, const char* format, Args&&... args) {
    lua_pushstring(L, realm::util::format(format, args...).c_str());
    return lua_error(L);

    // TODO:
    // Compare to luaL_error
}


// Informs the user about the last error that occured in
// Realm through realm_get_last_error.
int _inform_realm_error(lua_State* L);
