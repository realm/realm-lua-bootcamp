#define realm_userdata_t struct realm_lua_userdata*

#include <filesystem>

#include "realm_app.hpp"
#include "realm_util.hpp"
#include "curl_http_transport.hpp"

static void on_register_email_complete(realm_lua_userdata* userdata, const realm_app_error_t* error) {
    lua_State* L = userdata->L;

    // Get the Lua callback function from the register and push onto the stack.
    lua_rawgeti(L, LUA_REGISTRYINDEX, userdata->callback_reference);

    // Push an error message onto the stack as an argument to the user's callback if needed.
    int num_callback_args = 0;
    if (error) {
        lua_pushstring(L, error->message);
        num_callback_args = 1;
    }

    // Call the user's callback function.
    int status = lua_pcall(L, num_callback_args, 0, 0);
    log_lua_error(L, status);
}

static void on_log_in_complete(realm_lua_userdata* userdata, realm_user_t* user_arg, const realm_app_error_t* error) {
    lua_State* L = userdata->L;

    // Get the Lua callback function from the register and push onto the stack.
    lua_rawgeti(L, LUA_REGISTRYINDEX, userdata->callback_reference);

    // Push the realm user onto the stack and set its metatable (or push nil as
    // the realm user and an error message) as argument(s) to the user's callback.
    int num_callback_args = 1;
    if (user_arg) {
        realm_user_t** user = static_cast<realm_user_t**>(lua_newuserdata(L, sizeof(realm_user_t*)));
        luaL_setmetatable(L, RealmHandle);
        *user = (realm_user_t*)realm_clone(user_arg);
    }
    else {
        lua_pushnil(L);
        lua_pushstring(L, error->message);
        num_callback_args = 2;
    }

    // Call the user's callback function.
    int status = lua_pcall(L, num_callback_args, 0, 0);
    log_lua_error(L, status);
}

static int lib_realm_app_create(lua_State* L) {
    // Get arguments needed to create configuration objects.
    const char* app_id = (const char*)lua_tostring(L, 1);
    realm_http_transport_t* http_transport = make_curl_http_transport();

    // Get configuration objects needed to create a realm app.
    realm_app_config_t* app_config = realm_app_config_new(app_id, http_transport);
    realm_app_config_set_platform(app_config, "Realm Lua");
    realm_app_config_set_sdk(app_config, "Lua");
    realm_app_config_set_sdk_version(app_config, "0.0.1-alpha");
    realm_app_config_set_platform_version(app_config, "macOS");

    // NOTE: For production this has to provide an explicit encryption key.
    realm_sync_client_config_t* sync_client_config = realm_sync_client_config_new();
    realm_sync_client_config_set_base_file_path(sync_client_config, std::filesystem::current_path().c_str());
    realm_sync_client_config_set_metadata_mode(sync_client_config, RLM_SYNC_CLIENT_METADATA_MODE_PLAINTEXT);

    // Create and push the realm app onto the stack and set its metatable.
    realm_app_t** app = static_cast<realm_app_t**>(lua_newuserdata(L, sizeof(realm_app_t*)));
    luaL_setmetatable(L, RealmHandle);
    *app = realm_app_create(app_config, sync_client_config);

    realm_release(http_transport);
    realm_release(app_config);
    realm_release(sync_client_config);

    if (!*app) {
        return _inform_realm_error(L);
    }

    return 1;
}

static int lib_realm_app_email_password_provider_client_register_email(lua_State* L) {
    // Get arguments from the stack.
    realm_app_t** app = (realm_app_t**)lua_touserdata(L, 1);
    const char* email = (const char*)lua_tostring(L, 2);
    size_t password_len;
    const char* password_data = (const char*)lua_tolstring(L, 3, &password_len);
    realm_string_t password {
        .data = password_data,
        .size = password_len,
    };

    // Pop last argument/top of stack (the Lua function) from the stack and save a
    // reference to it in the register. "callback_reference" is the register location.
    int callback_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    // Create a pointer to userdata for use in the callback when called.
    realm_lua_userdata* userdata = new realm_lua_userdata;
    userdata->L = L;
    userdata->callback_reference = callback_reference;

    // Register the email.
    bool status = realm_app_email_password_provider_client_register_email(
        *app,
        email,
        password,
        on_register_email_complete,
        userdata,
        free_lua_userdata
    );
    if (!status) {
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_app_credentials_new_email_password(lua_State* L) {
    // Get arguments from the stack.
    const char* email = (const char*)lua_tostring(L, 1);
    size_t password_len;
    const char* password_data = (const char*)lua_tolstring(L, 2, &password_len);
    realm_string_t password {
        .data = password_data,
        .size = password_len,
    };

    // Create and push the realm app credentials onto the stack and set its metatable.
    realm_app_credentials_t** app_credentials = static_cast<realm_app_credentials_t**>(lua_newuserdata(L, sizeof(realm_app_credentials_t*)));
    luaL_setmetatable(L, RealmHandle);
    *app_credentials = realm_app_credentials_new_email_password(email, password);

    return 1;
}

static int lib_realm_app_log_in_with_credentials(lua_State* L) {
    // Get arguments from the stack.
    realm_app_t** app = (realm_app_t**)lua_touserdata(L, 1);
    realm_app_credentials_t** app_credentials = (realm_app_credentials_t**)lua_touserdata(L, 2);

    // Pop last argument/top of stack (the Lua function) from the stack and save a
    // reference to it in the register. "callback_reference" is the register location.
    int callback_reference = luaL_ref(L, LUA_REGISTRYINDEX);

    // Create a pointer to userdata for use in the callback when called.
    realm_lua_userdata* userdata = new realm_lua_userdata;
    userdata->L = L;
    userdata->callback_reference = callback_reference;

    // Log in.
    int status = realm_app_log_in_with_credentials(
        *app,
        *app_credentials,
        on_log_in_complete,
        userdata,
        free_lua_userdata
    );
    if (!status) {
        return _inform_realm_error(L);
    }

    return 0;
}

static int lib_realm_app_get_current_user(lua_State* L) {
    // Get argument.
    realm_app_t** app = (realm_app_t**)lua_touserdata(L, 1);

    // Create and push the user (or nil) onto the stack and set its metatable.
    realm_user_t* user = realm_app_get_current_user(*app);
    if (user) {
        realm_user_t** userData = static_cast<realm_user_t**>(lua_newuserdata(L, sizeof(realm_user_t*)));
        luaL_setmetatable(L, RealmHandle);
        *userData = user;
    }
    else {
        lua_pushnil(L);
    }

    return 1;
}

static int lib_realm_app_credentials_new_anonymous(lua_State* L) {
    // Get argument if user sent one.
    bool reuse_credentials;
    if (lua_gettop(L) == 1) {
        reuse_credentials = lua_toboolean(L, 1);
    }

    // Create and push the realm app credentials onto the stack and set its metatable.
    realm_app_credentials_t** app_credentials = static_cast<realm_app_credentials_t**>(lua_newuserdata(L, sizeof(realm_app_credentials_t*)));
    luaL_setmetatable(L, RealmHandle);
    *app_credentials = realm_app_credentials_new_anonymous(reuse_credentials);

    return 1;
}

extern "C" int luaopen_realm_app_native(lua_State* L) {
    luaL_Reg funcs[] = {
        {"realm_app_create",                                        lib_realm_app_create},
        {"realm_app_credentials_new_anonymous",                     lib_realm_app_credentials_new_anonymous},
        {"realm_app_credentials_new_email_password",                lib_realm_app_credentials_new_email_password},
        {"realm_app_email_password_provider_client_register_email", lib_realm_app_email_password_provider_client_register_email},
        {"realm_app_get_current_user",                              lib_realm_app_get_current_user},
        {"realm_app_log_in_with_credentials",                       lib_realm_app_log_in_with_credentials},
        {NULL, NULL}
    };
    luaL_newlib(L, funcs);

    return 1;
}
