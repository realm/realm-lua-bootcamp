#include <iostream>
#include <vector>
#include <lua.hpp>

#include "realm_util.hpp"
#include "realm_schema.hpp"

// Checks whether given fullString ends with ending 
bool ends_with (const std::string_view& full_string, const std::string_view& ending) {
    if (full_string.length() >= ending.length()) {
        return (0 == full_string.compare (full_string.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

static void _parse_property_type(lua_State* L, realm_property_info_t& prop, std::string_view type) {
    if (!type.size()) {
        _inform_error(L, "");
    }
    prop.flags = RLM_PROPERTY_NORMAL;
    
    if (ends_with(type, "[]")) {
        prop.collection_type = RLM_COLLECTION_TYPE_LIST;
        type = type.substr(0, type.size() - 2);
    }

    if (ends_with(type, "<>")) {
        prop.collection_type = RLM_COLLECTION_TYPE_SET;
        type = type.substr(0, type.size() - 2);
    }

    if (ends_with(type, "?")) {
        prop.flags |= RLM_PROPERTY_NULLABLE;
        type = type.substr(0, type.size() - 1);
    }

    if (ends_with(type, "{}")) {
        prop.collection_type = RLM_COLLECTION_TYPE_DICTIONARY;
        type = type.substr(0, type.size() - 2);

        if (type == "") {
            prop.type = RLM_PROPERTY_TYPE_MIXED; 
            prop.flags |= RLM_PROPERTY_NULLABLE;
            return;
        }
    }

    if (type == "bool") {
        prop.type = RLM_PROPERTY_TYPE_BOOL;
    }
    else if (type == "mixed") {
        prop.type = RLM_PROPERTY_TYPE_MIXED;
    }
    else if (type == "int") {
        prop.type = RLM_PROPERTY_TYPE_INT;
    }
    else if (type == "float") {
        prop.type = RLM_PROPERTY_TYPE_FLOAT;
    }
    else if (type == "double") {
        prop.type = RLM_PROPERTY_TYPE_DOUBLE;
    }
    else if (type == "string") {
        prop.type = RLM_PROPERTY_TYPE_STRING;
    }
    else if (type == "date") {
        prop.type = RLM_PROPERTY_TYPE_TIMESTAMP;
    }
    else if (type == "data") {
        prop.type = RLM_PROPERTY_TYPE_BINARY;
    }
    else if (type == "decimal128") {
        prop.type = RLM_PROPERTY_TYPE_DECIMAL128;
    }
    else if (type == "objectId") {
        prop.type = RLM_PROPERTY_TYPE_OBJECT_ID;
    }
    else if (type == "uuid") {
        prop.type = RLM_PROPERTY_TYPE_UUID;
    }
    else {
        _inform_error(L, "Unknown type");
        prop.type = RLM_PROPERTY_TYPE_MIXED;
    }
}

realm_schema_t* _parse_schema(lua_State* L) {
    size_t classes_len = lua_rawlen(L, -1);
    
    // Array of classes and a two-dimensional
    // array of properties for every class.
    realm_class_info_t classes[classes_len];
    memset(classes, 0, sizeof(realm_class_info_t)*classes_len);
    
    const realm_property_info_t* properties[classes_len];
    // 2D vector of class properties to act as a memory buffer
    // for the actual properties array.
    std::vector<std::vector<realm_property_info_t>> properties_vector = {};

    int argument_index = lua_gettop(L);
    for (size_t i = 1; i <= classes_len; i++) {
        lua_rawgeti(L, argument_index, i);

        // Use name field to create initial class info
        lua_getfield(L, -1, "name");
        realm_class_info_t& class_info = classes[i-1];
        class_info.name = lua_tostring(L, -1);
        class_info.primary_key = "";
        lua_pop(L, 1);

        // Get properties and iterate through them
        lua_getfield(L, -1, "properties");
        luaL_checktype(L, -1, LUA_TTABLE);

        // Iterate through key-values of a specific class' properties table.
        // (Push nil since lua_next starts by popping.)
        std::vector<realm_property_info_t>& class_properties = *properties_vector.emplace({});
        lua_pushnil(L);
        while(lua_next(L, -2) != 0) {
            // Copy the key.
            lua_pushvalue(L, -2);
            // The copied key.
            luaL_checktype(L, -1, LUA_TSTRING);
            // The value.
            luaL_checktype(L, -2, LUA_TSTRING);

            realm_property_info_t& property_info = class_properties.emplace_back(realm_property_info_t{
                .name = lua_tostring(L, -1),
                // TODO?: add support for this
                .public_name = "",
                .link_target = "",
                .link_origin_property_name = "",
            });
            _parse_property_type(L, property_info, lua_tostring(L, -2));
            lua_pop(L, 2);
        }
        // Drop the properties field.
        lua_pop(L, 1);
         
        // Add the parsed class and property information to the array.
        class_info.num_properties = class_properties.size();        
        properties[i-1] = class_properties.data();
    }
    // Pop the last class index.
    lua_pop(L, 1);

    return realm_schema_new(classes, classes_len, properties);
}