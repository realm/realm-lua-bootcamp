local native = require "realm.native"
local scheduler = require "realm.scheduler"

---@module '.classes'

local RealmObject = require "realm.object"
local RealmResults = require "realm.results"

---@classmod realm
---@class Realm
---@field _handle userdata The realm userdata.
---@field _schema table<string, Realm.Schema.ClassInformation> The schema used when opening the realm.
---@field _childHandles userdata[] The userdata associated with the opened realm.
local Realm = {}
Realm.__index = Realm

function Realm:__gc()
    self:close()
end

function Realm:__close()
    self:close()
end

---Get the class information if it exists in schema, otherwise throw an error. 
---@param className string The class name.
---@param realm Realm The realm.
---@return Realm.Schema.ClassInformation
local function _safeGetClass(realm, className)
    local classInfo = realm._schema[className]
    if classInfo == nil then
        print(debug.traceback())
        error("Class ".. className .. " not found in schema");
    end

    return classInfo
end

---@generic T
---@param writeCallback fun(): T The callback performing the changes to apply to the realm.
---@return T
function Realm:write(writeCallback)
    native.realm_begin_write(self._handle)
    local status, result = xpcall(writeCallback, debug.traceback)
    if (status) then
        native.realm_commit_transaction(self._handle)
        return result
    end

    native.realm_cancel_transaction(self._handle)
    error(result)
end

---@param className string The class name.
---@param values table? The values to apply to the object.
---@param handle userdata? The realm object userdata.
---@generic T : Realm.Object
---@return T
function Realm:create(className, values, handle)
    return RealmObject._new(self, _safeGetClass(self, className), values, handle)
end

---Explicitly close this realm and its associated userdata (release native resources).
function Realm:close()
    for _, handle in ipairs(self._childHandles) do
        native.realm_release(handle)
    end
    native.realm_release(self._handle)
end

---@param object Realm.Object The object.
function Realm:delete(object)
    return native.realm_object_delete(object._handle)
end

---@param object Realm.Object The object.
function Realm:isValid(object)
    return native.realm_object_is_valid(object._handle)
end

---@param className string The class name.
---@return Realm.Results
function Realm:objects(className)
    local classInfo = _safeGetClass(self, className)
    local resultHandle = native.realm_object_get_all(self._handle, className)

    return RealmResults._new(self, resultHandle, classInfo)
end

---@param config Realm.Config The configuration for opening the realm.
---@return Realm
function Realm.open(config)
    local scheduler = config.scheduler and native.realm_clone(config.scheduler) or scheduler.defaultFactory()
    local _handle, _schema = native.realm_open(config, scheduler)
    native.realm_release(scheduler)
    local self = setmetatable({
        _handle = _handle,
        _schema = _schema,
        _childHandles = setmetatable({}, { __mode = "v"}) -- A table of weak references.
    }, Realm)

    return self
end

return Realm
