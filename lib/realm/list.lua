local native = require "_realm_native"
local RealmObject = require "realm.object"

---@class RealmList
---@field class Realm.Schema.ClassInformation
---@field _handle userdata
---@field _realm Realm
local RealmList = {}

function RealmList:new(realm ,handle, classInfo)
    local list = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
    }
    table.insert(realm._childHandles, list._handle)
    list = setmetatable(list, RealmList)
    return list 
end

function RealmList:__index(key)
    -- TODO: for now it's assumed that we only get objects
    local handle = native.realm_list_get(self._handle, self._realm._handle, key - 1)
    return RealmObject:new(self._realm, self.class, {}, handle)
end

function RealmList:__newindex(index, value)
    native.realm_list_insert(self._handle, index - 1, value._handle)
end

function RealmList:__len()
    return native.realm_list_size(self._handle)
end

return RealmList
