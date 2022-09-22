#include "realm_user.hpp"

int _lib_realm_user_log_out(lua_State* L) {

    // realm.h

    // RLM_API bool realm_user_log_out(realm_user_t*);


    // NOTE: There is also:
    // RLM_API bool realm_app_log_out(realm_app_t* app, realm_user_t* user, realm_app_void_completion_func_t callback,
    //                            realm_userdata_t userdata, realm_free_userdata_func_t userdata_free);

    return 0;
}

int _lib_realm_user_get_id(lua_State* L) {

    // realm.h

    // RLM_API const char* realm_user_get_identity(const realm_user_t* user) RLM_API_NOEXCEPT;

    return 1;
}
