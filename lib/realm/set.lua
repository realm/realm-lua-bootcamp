local native = require "realm.native"

---@class RealmSet
---@field class Realm.Schema.ClassInformation The class information.
---@field remove function removes the object from the set.
---@field _handle userdata The realm set userdata.
---@field _realm Realm The realm.
local RealmSet = {}

---@param self Realm.Set The realm set.
---@param value any the value to remove.
---@return boolean
local function remove(self, value)
    return native.realm_set_erase(self._handle, value)
end
---@param realm Realm The realm.
---@param handle userdata The realm set userdata.
---@param classInfo Realm.Schema.ClassInformation The class information.
---@return Realm.Set
function RealmSet:new(realm, handle, classInfo)
    local set = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
        remove = remove
    }
    table.insert(realm._childHandles, set._handle)
    return setmetatable(set, RealmSet)
end

--- @param value any The value to lookup in the set.
function RealmSet:__index(value)
    if type(value) == "table" then
        value = value._handle
    end
    return native.realm_set_find(self._handle, value)
end

--- @param value any The value to insert into the set.
function RealmSet:__newindex(_, value)
    if type(value) == "table" then
        value = value._handle
    end
    native.realm_set_insert(self._handle, value)
end

function RealmSet:__len()
    return native.realm_set_size(self._handle)
end

return RealmSet
