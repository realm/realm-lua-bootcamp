local native = require "realm.native"

---@class RealmSet
---@field class Realm.Schema.ClassInformation The class information.
---@field _handle userdata The realm set userdata.
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

--- @param value any The value to look up in the set.
function RealmSet:__index(value)
    if type(value) == "table" then
        value = value._handle
    end
    return native.realm_set_find(self._handle, value)
end

--- @param entry string | number | boolean | Realm.Object The entry to add to the set.
--- @param value true | nil True implies that the entry should be inserted in the set. Nil means deletion.
function RealmSet:__newindex(entry, value)
    if type(entry) == "table" then
        entry = entry._handle
    end
    if value == nil then
        native.realm_set_erase(self._handle, entry)
        return
    end
    if not value == true then
        error("Entries in a set can only have a value of true")
        return
    end
    native.realm_set_insert(self._handle, entry)
end

function RealmSet:__len()
    return native.realm_set_size(self._handle)
end

return RealmSet
