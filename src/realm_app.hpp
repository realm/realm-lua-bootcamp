#include <lua.hpp>

int _lib_realm_app_create(lua_State* L);

int _lib_realm_app_register_email(lua_State* L);

int _lib_realm_app_credentials_new_email_password(lua_State* L);

int _lib_realm_app_log_in(lua_State* L);

int _lib_realm_app_get_current_user(lua_State* L);
