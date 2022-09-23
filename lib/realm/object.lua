local native = require "realm.native"

---@module '.init'

---@class Realm.Object
---@field _handle userdata
---@field _realm Realm
---@field class Realm.Schema.ClassInformation
---@field addListener fun(self: Realm.Object, cb: Realm.ObjectChanges.Callback) : Realm.Handle
local RealmObject = {}

---@param self Realm.Object
---@param onObjectChange Realm.ObjectChanges.Callback
---@return Realm.Handle Notification token
local function addListener(self, onObjectChange)
    -- Create a listener that is passed to cpp which, when called, in turn calls
    -- the user's listener (onObjectChange). This makes it possible to pass the
    -- object (self) from Lua instead of cpp.
    local function listener(changes)
        onObjectChange(self, changes)
    end
    local notificationToken = native.realm_object_add_listener(self._handle, listener)
    table.insert(self._realm._childHandles, notificationToken)
    return notificationToken
end

---@param realm Realm
---@param classInfo Realm.Schema.ClassInformation
---@param values table<string, any>?
---@param handle userdata?
---@return Realm.Object 
function RealmObject:new(realm, classInfo, values, handle)
    local noPrimaryKey = (classInfo.primaryKey == nil or classInfo.primaryKey == '')
    local hasValues = values ~= nil
    if handle == nil then
        if (noPrimaryKey) then
            handle = native.realm_object_create(realm._handle, classInfo.key)
        else
            if not hasValues or values[classInfo.primaryKey] == nil then
                error("Primary key not set at declaration")
                return {}
            end
            handle = native.realm_object_create_with_primary_key(realm._handle, classInfo.key, values[classInfo.primaryKey])
            -- Remove primaryKey from values to insert since it's already in the created object
            values[classInfo.primaryKey] = nil
        end
        if hasValues then
            -- Insert rest of the values into the created object
            for prop, value in pairs(values) do
                native.realm_set_value(realm._handle, handle, classInfo.properties[prop].key, value)
            end
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

---@param schema table<string, Realm.Schema.ClassInformation>
---@param classKey number
---@return Realm.Schema.ClassInformation
local function _findClass(schema, classKey)
    for _, classInfo in pairs(schema) do
        if classInfo.key == classKey then
            return classInfo
        end
    end
    error("Given a class key without a cached class")
    return {}
end

--- @param prop string 
function RealmObject:__index(prop)
    -- refClass is only returned if the field is a reference to an object.
    local value, refClassKey = native.realm_get_value(self._realm._handle, self._handle, self.class.properties[prop].key)
    if refClassKey ~= nil then
        return RealmObject:new(self._realm, _findClass(self._realm._schema, refClassKey), nil, value)
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
