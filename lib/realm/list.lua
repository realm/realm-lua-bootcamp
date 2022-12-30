local native = require "realm.native"
local RealmObject = require "realm.object"

---@class RealmList
---@field class Realm.Schema.ClassInformation The class information.
---@field _handle userdata The realm list userdata.
---@field _realm Realm The realm.
local RealmList = {}

---@param realm Realm The realm.
---@param handle userdata The realm list userdata.
---@param classInfo Realm.Schema.ClassInformation The class information.
---@return Realm.List
function RealmList:new(realm, handle, classInfo)
    local list = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
    }
    table.insert(realm._childHandles, list._handle)

    return setmetatable(list, RealmList)
end

function RealmList:__index(key)
    local value = native.realm_list_get(self._handle, self._realm._handle, key - 1)
    if type(value) == "userdata" then
        return RealmObject._new(self._realm, self.class, {}, value)
    end
    return value
end

function RealmList:__newindex(index, value)
    -- assigning a value to nil means a deletion
    if value == nil then
        native.realm_list_erase(self._handle, index - 1)
        return
    end
    -- A realm object type in Lua will correspond to a "table"
    if type(value) == "table" then
        value = value._handle
    end
    native.realm_list_insert(self._handle, index - 1, value)
end

function RealmList:__len()
    return native.realm_list_size(self._handle)
end

return RealmList
