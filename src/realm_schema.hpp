#include <lua.hpp>
#include <realm.h>

realm_schema_t* _parse_schema(lua_State*);

// Push current schema information onto the Lua stack, 
// in form of a map of class name to class info from 
// an already open Realm. Useful for caching purposes. 
// The Lua object is in form: 
// [class_name] => { key, property: ([property_name] => property_info) }
void _push_schema_info(lua_State*, const realm_t*);