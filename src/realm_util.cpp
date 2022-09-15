#ifndef REALM_LUA_UTIL_H
#define REALM_LUA_UTIL_H

#include <realm.h>
#include "realm_util.hpp"

int _inform_realm_error(lua_State* L) {
    realm_error_t error;
    realm_get_last_error(&error);
    return _inform_error(L, error.message);
}

#endif
