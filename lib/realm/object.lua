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
---@param classInfo Realm.Schema.ClassInformation
---@return RealmObject 
function RealmObject:new(realm, classInfo, values, handle)
    local noPrimaryKey = (classInfo.primaryKey == nil or classInfo.primaryKey == '')

    if handle == nil then
        if (noPrimaryKey) then
            handle = native.realm_object_create(realm._handle, classInfo.key)
        else
            handle = native.realm_object_create_with_primary_key(realm._handle, classInfo.key, values[classInfo.primaryKey])
            -- Remove primaryKey from values to insert since it's already in the created object
            values[classInfo.primaryKey] = nil
        end
            -- Insert rest of the values into the created object
        for prop, value in pairs(values) do
            native.realm_set_value(realm._handle, handle, classInfo.properties[prop].key, value)
        end
    end 
    local object = {
        _handle = handle,
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
    -- refClass is only returned if the field is a reference to an object.
    local value, refClass = native.realm_get_value(self._realm._handle, self._handle, self.class.properties[prop].key)
    if refClass ~= nil then
        return RealmObject:new(self._realm, self._realm._schema[refClass], value)
    end
    return value
end

--- @param prop string
--- @param value any
function RealmObject:__newindex(prop, value)
    -- Ensure only Realm Objects are set for references to fields.
    if (type(value) == "table") then
        if getmetatable(value) == RealmObject then
            native.realm_set_value(self._realm._handle, self._handle, self.class.properties[prop].key, value._handle)
        else
            error('Only other Realm Objects can be set as references for the property "' .. prop .. '"')
        end
        return
    end
    native.realm_set_value(self._realm._handle, self._handle, self.class.properties[prop].key, value)
end

return RealmObject
