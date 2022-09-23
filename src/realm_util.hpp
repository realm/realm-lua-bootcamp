#ifndef REALM_LUA_UTIL_H
#define REALM_LUA_UTIL_H
#include <lua.hpp>
#include <string_view>

#include <realm.h>
#include <realm/util/to_string.hpp>

static const char* RealmHandle = "_realm_handle";

template <typename... Args>
int _inform_error(lua_State* L, const char* format, Args&&... args) {
    lua_pushstring(L, realm::util::format(format, args...).c_str());
    return lua_error(L);
}

// Informs the user about the last error that occured in
// Realm through realm_get_last_error.
int _inform_realm_error(lua_State* L);

inline int log_lua_error(lua_State* L, int status) {
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        lua_writestringerror("%s\n", msg);
        lua_pop(L, 1);
    }
    return status;
}

// Converts a lua value on the stack into a corresponding realm value
std::optional<realm_value_t> lua_to_realm_value(lua_State* L, int arg_index);

// Converts a realm value to a corresponding lua value and pushes it onto the stack
int realm_to_lua_value(lua_State* L, realm_t* realm, realm_value_t value);

// Fetches property info based on an object and its property name
std::optional<realm_property_info_t> get_property_info_by_name(lua_State* L, realm_t* realm, realm_object_t* object, const char* property_name);

// Fetches property info based on an object and its property key
std::optional<realm_property_info_t> get_property_info_by_key(lua_State* L, realm_t* realm, realm_object_t* object, realm_property_key_t property_key);

// Checks whether given fullString ends with ending 
inline bool ends_with (const std::string_view& full_string, const std::string_view& ending) {
    if (full_string.length() >= ending.length()) {
        return (0 == full_string.compare (full_string.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

inline std::string_view lua_tostringview(lua_State* L, int index) {
    size_t len;
    const char* data = lua_tolstring(L, index, &len);
    if (data) {
        return std::string_view(data, len);
    }

    return {};
}

#endif
