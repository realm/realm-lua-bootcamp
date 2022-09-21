local native = require "_realm_native"

---@module '.init'

---@class RealmObject
---@field _handle userdata
---@field _realm Realm
---@field class Realm.Schema.ClassInformation
---@field addListener function
local RealmObject = {}

local function addListener(self, onObjectChange)
    -- Create a listener that is passed to cpp which, when called, in turn calls
    -- the user's listener (onObjectChange). This makes it possible to pass the
    -- object (self) from Lua instead of cpp.
    local function listener(changes)
        onObjectChange(self, changes)
    end
    return native.realm_object_add_listener(self._handle, listener)
end

---@param realm Realm
---@return RealmObject 
function RealmObject:new(realm, classInfo, values)
    local noPrimaryKey = (classInfo.primaryKey == nil or classInfo.primaryKey == '')

    local objectHandle
    if (noPrimaryKey) then
        objectHandle = native.realm_object_create(realm._handle, classInfo.key)
    else
        objectHandle = native.realm_object_create_with_primary_key(realm._handle, classInfo.key, values[classInfo.primaryKey])
        -- Remove primaryKey from values to insert since it's already in the created object
        values[classInfo.primaryKey] = nil
    end
        -- Insert rest of the values into the created object
    for prop, value in pairs(values) do
        native.realm_set_value(realm._handle, objectHandle, classInfo.properties[prop].key, value)
    end
    
    local object = {
        _handle = objectHandle,
        _realm = realm,
        class = classInfo,
        addListener = addListener,
    }
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
