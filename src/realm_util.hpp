#include <lua.hpp>

#include <realm.h>
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

// Converts a lua value on the stack into a corresponding realm value
std::optional<realm_value_t> lua_to_realm_value(lua_State* L, int arg_index);

// Converts a realm value to a corresponding lua value and pushes it onto the stack
int realm_to_lua_value(lua_State* L, realm_value_t value);

// Fetches property info based on an object and its property name
std::optional<realm_property_info_t> get_property_info(lua_State* L, realm_t* realm, realm_object_t* object, const char* property_name);