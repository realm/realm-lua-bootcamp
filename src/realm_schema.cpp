#include <iostream>
#include <vector>
#include <lua.hpp>

#include <realm/object-store/c_api/types.hpp>
#include <realm/object-store/c_api/conversion.hpp>

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

static void _parse_property_type(lua_State* L, realm_property_info_t& prop, std::string_view type, std::vector<std::string>& strings) {
    if (!type.size()) {
        _inform_error(L, "");
    }
    
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
        prop.type = RLM_PROPERTY_TYPE_OBJECT;
        prop.link_target = strings.emplace_back(type).c_str();
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
    std::vector<std::vector<realm_property_info_t>> properties_vector;
    std::vector<std::string> properties_strings;

    int argument_index = lua_gettop(L);
    for (size_t i = 1; i <= classes_len; i++) {
        lua_rawgeti(L, argument_index, i);

        // Use name field to create initial class info
        lua_getfield(L, -1, "name");
        realm_class_info_t& class_info = classes[i-1];
        class_info.name = lua_tostring(L, -1);
        lua_pop(L, 1);

        // Check if primaryKey is specified for a class in the schema
        class_info.primary_key = lua_getfield(L, -1, "primaryKey") ? lua_tostring(L, -1) : "";
        lua_pop(L, 1);

        // Get properties and iterate through them
        lua_getfield(L, -1, "properties");
        luaL_checktype(L, -1, LUA_TTABLE);

        // Iterate through key-values of a specific class' properties table.
        // (Push nil since lua_next starts by popping.)
        std::vector<realm_property_info_t>& class_properties = properties_vector.emplace_back(std::vector<realm_property_info_t>());
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
                // Primary keys get implicitly indexed
                .flags = strcmp(lua_tostring(L, -1), class_info.primary_key) == 0 
                    ? RLM_PROPERTY_PRIMARY_KEY
                    : RLM_PROPERTY_NORMAL

            });
            _parse_property_type(L, property_info, lua_tostring(L, -2), properties_strings);
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

void _push_schema_info(lua_State* L, const realm_t* realm) {
    const realm::Schema& schema = (*realm)->schema();
    
    lua_newtable(L);

    const char* class_name;
    const char* property_name;
    for(realm::ObjectSchema class_info : schema) {
        lua_newtable(L);
        
        // Set fields
        class_name = class_info.name.c_str();
        lua_pushstring(L, class_name);
        lua_setfield(L, -2, "name");

        lua_pushinteger(L, class_info.table_key.value);
        lua_setfield(L, -2, "key");

        if (class_info.primary_key.size() > 0) {
            lua_pushstring(L, class_info.primary_key.c_str());
            lua_setfield(L, -2, "primaryKey");
        }

        // Create a property lookup table where [property_name] => property_info
        lua_newtable(L);
        for(realm::Property property_info : class_info.persisted_properties) {
            lua_newtable(L);
            
            property_name = property_info.name.c_str();
            lua_pushstring(L, property_name);
            lua_setfield(L, -2, "name");

            auto* key_userdata = static_cast<realm_property_key_t*>(lua_newuserdata(L, sizeof(realm_property_key_t))); 
            *key_userdata = property_info.column_key.value;
            lua_setfield(L, -2, "key");
            
            lua_pushinteger(L, realm::c_api::to_capi(property_info.type));
            lua_setfield(L, -2, "type");
 
            lua_setfield(L, -2, property_name);
        }
        lua_setfield(L, -2, "properties");
        
        // Set the field on the greater schema info table
        // for easy lookup by class name.
        lua_setfield(L, -2, class_name);
    }
}
