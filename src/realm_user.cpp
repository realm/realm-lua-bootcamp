#include "realm_user.hpp"
#include "realm_util.hpp"

int _lib_realm_user_log_out(lua_State* L) {
    // Get argument.
    realm_user_t** user = (realm_user_t**)lua_touserdata(L, 1);

    // Log out.
    int status = realm_user_log_out(*user);
    if (!status) {
        return _inform_realm_error(L);
    }

    return 0;
}

int _lib_realm_user_get_id(lua_State* L) {
    // Get argument.
    const realm_user_t** user = (const realm_user_t**)lua_touserdata(L, 1);

    // Push the user's ID onto the stack.
    lua_pushstring(L, realm_user_get_identity(*user));

    return 1;
}
