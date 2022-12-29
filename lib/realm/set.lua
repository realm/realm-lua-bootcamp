local native = require "realm.native"
local RealmObject = require "realm.object"

---@class RealmSet
---@field class Realm.Schema.ClassInformation The class information.
---@field _handle userdata The realm list userdata.
---@field _realm Realm The realm.
local RealmSet = {}

---@param realm Realm The realm.
---@param handle userdata The realm set userdata.
---@param classInfo Realm.Schema.ClassInformation The class information.
---@return Realm.Set

function RealmSet:new(realm, handle, classInfo)
    local set = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
    }
    table.insert(realm._childHandles, set._handle)
    return setmetatable(set, RealmSet)
end

function RealmSet:__newindex(value)
    native.realm_set_insert(self._handle, value._handle)
end
