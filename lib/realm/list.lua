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
    -- TODO: For now it's assumed that we only get objects.
    local handle = native.realm_list_get(self._handle, self._realm._handle, key - 1)

    return RealmObject._new(self._realm, self.class, {}, handle)
end

function RealmList:__newindex(index, value)
    native.realm_list_insert(self._handle, index - 1, value._handle)
end

function RealmList:__len()
    return native.realm_list_size(self._handle)
end

return RealmList
