#include "realm_app.hpp"

int _lib_realm_app_create(lua_State* L) {

    // realm.h:

    // Create a realm_app_config_t
    // RLM_API realm_app_config_t* realm_app_config_new(const char* app_id,
    //                                              const realm_http_transport_t* http_transport) RLM_API_NOEXCEPT;

    // Possibly set more fields on the config object (if also passed from Lua) (but it's optional)
    // RLM_API void realm_app_config_set_base_url(realm_app_config_t*, const char*) RLM_API_NOEXCEPT;
    // RLM_API void realm_app_config_set_local_app_name(realm_app_config_t*, const char*) RLM_API_NOEXCEPT;
    // RLM_API void realm_app_config_set_local_app_version(realm_app_config_t*, const char*) RLM_API_NOEXCEPT;

    // Create a realm_app_t
    // RLM_API realm_app_t* realm_app_create(const realm_app_config_t*, const realm_sync_client_config_t*);

    return 1;
}

int _lib_realm_app_register_email(lua_State* L) {

    // realm.h

    // RLM_API bool realm_app_email_password_provider_client_register_email(realm_app_t* app, const char* email,
    //                                                                  realm_string_t password,
    //                                                                  realm_app_void_completion_func_t callback,
    //                                                                  realm_userdata_t userdata,
    //                                                                  realm_free_userdata_func_t userdata_free);

    return 0;
}

int _lib_realm_app_credentials_new_email_password(lua_State* L) {

    // realm.h

    // RLM_API realm_app_credentials_t* realm_app_credentials_new_email_password(const char* email,
    //                                                                       realm_string_t password) RLM_API_NOEXCEPT;

    return 1;
}

int _lib_realm_app_log_in(lua_State* L) {

    // realm.h

    // RLM_API bool realm_app_log_in_with_credentials(realm_app_t* app, realm_app_credentials_t* credentials,
    //                                            realm_app_user_completion_func_t callback, realm_userdata_t userdata,
    //                                            realm_free_userdata_func_t userdata_free);

    return 0;
}
