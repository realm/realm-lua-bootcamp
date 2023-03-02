local native = require "realm.native"
local RealmObject = require "realm.object"

---@class RealmDictionary
---@field class Realm.Schema.ClassInformation The class information.
---@field _handle userdata The realm list userdata.
---@field _realm Realm The realm.
local RealmDictionary = {}

---@param realm Realm The realm.
---@param handle userdata The realm list userdata.
---@param classInfo Realm.Schema.ClassInformation The class information.
---@return Realm.Dictionary
function RealmDictionary:new(realm, handle, classInfo)
    local dictionary = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
    }
    table.insert(realm._childHandles, dictionary._handle)

    return setmetatable(dictionary, RealmDictionary)
end

--- @param key string The key to look up in the set.
function RealmDictionary:__index(key)
    local value = native.realm_dictionary_find(self._handle, key, self._realm._handle)
    if type(value) == "userdata" then
        return RealmObject._new(self._realm, self.class, {}, value)
    end
    return value
end

--- @param key string The key to look up in the set.
--- @param value string | number | boolean | Realm.Object The value to add to the dictionary.
function RealmDictionary:__newindex(key, value)
    if type(value) == "table" then
        value = value._handle
    end
    if value == nil then
        native.realm_dictionary_erase(self._handle, key)
        return
    end
    native.realm_dictionary_insert(self._handle, key, value)
end

function RealmDictionary:__len()
    return native.realm_dictionary_size(self._handle)
end

return RealmDictionary
