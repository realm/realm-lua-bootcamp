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

return RealmDictionary
