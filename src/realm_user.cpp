#include "realm_user.hpp"
#include "realm_util.hpp"

static int lib_realm_user_log_out(lua_State* L) {
    // Get argument.
    realm_user_t** user = (realm_user_t**)lua_touserdata(L, 1);

    // Log out.
    int status = realm_user_log_out(*user);
    if (!status) {
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_user_get_identity(lua_State* L) {
    // Get argument.
    const realm_user_t** user = (const realm_user_t**)lua_touserdata(L, 1);

    // Push the user's ID onto the stack.
    lua_pushstring(L, realm_user_get_identity(*user));

    return 1;
}

extern "C" int luaopen_realm_app_user_native(lua_State* L) {
    luaL_Reg funcs[] = {
        {"realm_user_log_out",      lib_realm_user_log_out},
        {"realm_user_get_identity", lib_realm_user_get_identity},
        {NULL, NULL}
    };
    luaL_newlib(L, funcs);
    return 1;
}
