local native = require "_realm_native"

---@module '.init'

---@class RealmObject
---@field _handle userdata
---@field _realm Realm
---@field class Realm.Schema.ClassInformation
---@field addListener function
local RealmObject = {}

---@param realm Realm
---@return RealmObject 
function RealmObject:new(realm, classInfo)
    local object = {
        _handle = native.realm_object_create(realm._handle, classInfo.key),
        _realm = realm,
        class = classInfo,
    }
    function object:addListener(onObjectChange)
        -- Create a listener that is passed to cpp which, when called, in turn calls
        -- the user's listener (onObjectChange). This makes it possible to pass the
        -- object (self) from Lua instead of cpp.
        local function listener(changes)
            onObjectChange(self, changes)
        end
        return native.realm_object_add_listener(object._handle, listener)
    end
    table.insert(realm._childHandles, object._handle)
    object = setmetatable(object, RealmObject)
    return object
end

--- @param prop string 
function RealmObject:__index(prop)
    return native.realm_get_value(self._realm._handle, self._handle, self.class.properties[prop].key)
end

--- @param prop string
--- @param value any
function RealmObject:__newindex(prop, value)
    native.realm_set_value(self._realm._handle, self._handle, self.class.properties[prop].key, value)
end

return RealmObject
