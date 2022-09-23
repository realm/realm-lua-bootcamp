#include "realm_util.hpp"

int _inform_realm_error(lua_State* L) {
    realm_error_t error;
    realm_get_last_error(&error);
    return _inform_error(L, error.message);
}

std::optional<realm_value_t> lua_to_realm_value(lua_State* L, int arg_index){
    // TODO: add support for lists as input
    if (lua_type(L, arg_index) == LUA_TNUMBER){
        // Value is either of type double or int, need further investigation
        if (lua_isinteger(L, arg_index)){
            return realm_value_t{
                .type = RLM_TYPE_INT,
                .integer = lua_tointeger(L, arg_index),
            };
        } else {
            return realm_value_t{
                .type = RLM_TYPE_DOUBLE,
                .dnum = lua_tonumber(L, arg_index),
            };
        }
    } else if (lua_type(L, arg_index) == LUA_TSTRING){
        return realm_value_t{
            .type = RLM_TYPE_STRING,
            .string.size = lua_rawlen(L, arg_index),
            .string.data = lua_tostring(L, arg_index),
        };
    } else if (lua_type(L, arg_index) == LUA_TBOOLEAN){
        return realm_value_t{
            .type = RLM_TYPE_BOOL,
            .boolean = static_cast<bool>(lua_toboolean(L, arg_index)),
        };
    } else if (lua_type(L, arg_index) == LUA_TUSERDATA) {
            // All userdata is meant to be a reference to a valid Realm Object
            luaL_checkudata(L, arg_index, RealmHandle);
            realm_object_t** realm_object = (realm_object_t**)lua_touserdata(L, arg_index);
            luaL_setmetatable(L, RealmHandle);
            return realm_value_t{
            .type = RLM_TYPE_LINK,
            .link = realm_object_as_link(*realm_object)
        };
    } else {
        _inform_error(L, "Uknown Lua type: %1", lua_typename(L, lua_type(L, arg_index)));
        return std::nullopt;
    }
}

int realm_to_lua_value(lua_State* L, realm_t* realm, realm_value_t value){
    switch (value.type)
    {
    case RLM_TYPE_NULL:
        lua_pushnil(L);
        break;
    case RLM_TYPE_INT:
        lua_pushinteger(L, value.integer);
        break;
    case RLM_TYPE_BOOL:
        lua_pushboolean(L, value.boolean);
        break;
    case RLM_TYPE_STRING:
        lua_pushstring(L, value.string.data);
        break;
    case RLM_TYPE_FLOAT:
        lua_pushnumber(L, value.fnum);
        break;
    case RLM_TYPE_DOUBLE:
        lua_pushnumber(L, value.dnum);
        break;
    case RLM_TYPE_LINK: {
        realm_object_t* object = realm_get_object(realm, value.link.target_table, value.link.target);
        realm_object_t** realm_object_pointer = static_cast<realm_object_t**>(lua_newuserdata(L, sizeof(realm_object_t*)));
        *realm_object_pointer = object;

        // Set the metatable of the results (top of stack) to that
        // of RealmHandle in order for it to be released via __gc.
        luaL_setmetatable(L, RealmHandle);
        
        lua_pushinteger(L, value.link.target_table);
        return 2;
    }
    default:
        return _inform_error(L, "Uknown realm type");
    }
    return 1;
}

std::optional<realm_property_info_t> get_property_info_by_name(lua_State* L, realm_t* realm, realm_object_t* object, const char* property_name){
    realm_property_info_t property_info;
    realm_class_key_t class_key = realm_object_get_table(object);
    bool found = false;
    if (!realm_find_property(realm, class_key, property_name, &found, &property_info)) {
        // Exception ocurred when fetching the property 
        _inform_realm_error(L);
        return std::nullopt;
    }
    if (!found){
        _inform_error(L, "Unable to fetch value from property %1", property_name);
        return std::nullopt;
    }
    return property_info;
}

std::optional<realm_property_info_t> get_property_info_by_key(lua_State* L, realm_t* realm, realm_object_t* object, realm_property_key_t property_key) {
    realm_property_info_t property_info;
    realm_class_key_t class_key = realm_object_get_table(object);
    if (!realm_get_property(realm, class_key, property_key, &property_info)) {
        // Exception ocurred when fetching the property 
        _inform_realm_error(L);
        return std::nullopt;
    }
    return property_info;
}

realm_lua_userdata::~realm_lua_userdata() = default;

void free_lua_userdata(realm_lua_userdata* userdata) {
    luaL_unref(userdata->L, LUA_REGISTRYINDEX, userdata->callback_reference);
    delete userdata;
}
