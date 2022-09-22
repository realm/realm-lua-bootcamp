local native = require "realm.native"

---@module '.classes'

local RealmObject = require "realm.object"
local RealmResults = require "realm.results"

--@classmod realm
---@class Realm
---@field _handle userdata
---@field _schema table<string, Realm.Schema.ClassInformation>
---@field _childHandles userdata[]
local Realm = {}
Realm.__index = Realm

function Realm:__gc()
    self:close()
end

---Checks whether class exists in schema, throws error if not. 
---@param className string
---@param realm Realm
---@return Realm.Schema.ClassInformation
local function _safeGetClass(realm, className)
    local classInfo = realm._schema[className]
    if classInfo == nil then
        error("Class ".. className .. " not found in schema");
        return {}
    end
    return classInfo
end

---@generic T
---@param writeCallback fun(): T
---@return T
function Realm:write(writeCallback)
    native.realm_begin_write(self._handle)
    local status, result = pcall(writeCallback)
    if (status) then
        native.realm_commit_transaction(self._handle)
        return result
    else
        native.realm_cancel_transaction(self._handle)
        error(result)
    end
end

---@param className string
---@param values table?
---@param handle userdata?
---@return Realm.Object
function Realm:create(className, values, handle)
    return RealmObject:new(self, _safeGetClass(self, className), values, handle)
end

---Explicitly close this realm, releasing its native resources
function Realm:close()
    for _, handle in ipairs(self._childHandles) do
        native.realm_release(handle)
    end
    native.realm_release(self._handle)
end

---@param object Realm.Object 
function Realm:delete(object)
    return native.realm_object_delete(object._handle)
end

---@param object Realm.Object 
function Realm:isValid(object)
    return native.realm_object_is_valid(object._handle)
end

---@param className string
---@return Realm.Results
function Realm:objects(className)
    local classInfo = _safeGetClass(self, className)
    local resultHandle = native.realm_object_get_all(self._handle, className)
    return RealmResults:new(self, resultHandle, classInfo)
end

---@param config Realm.Config
function Realm.open(config)
    local _handle, _schema = native.realm_open(config)
    local self = setmetatable({
        _handle = _handle,
        _schema = _schema,
        _childHandles = setmetatable({}, { __mode = "v"}) -- a table of weak references
    }, Realm)
    return self
end

return Realm
