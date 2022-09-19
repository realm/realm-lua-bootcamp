local native = require "_realm_native"

---@module '.classes'

local RealmObject = require "realm.object"
local RealmResults = require "realm.results"

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
---@return Realm.Schema.ClassInformation?
local function _safeGetClass(realm, className)
    local classInfo = realm._schema[className]
    if classInfo == nil then
        error("Class not found in schema");
        return nil
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
---@return RealmObject?
function Realm:create(className)
    return RealmObject:new(self, _safeGetClass(self, className))
end

---Explicitly close this realm, releasing its native resources
function Realm:close()
    for _, handle in ipairs(self._childHandles) do
        native.realm_release(handle)
    end
    native.realm_release(self._handle)
end

---@param className string
---@return RealmResults
function Realm:objects(className)
    local classInfo = _safeGetClass(self, className)
    local result_handle = native.realm_object_get_all(self._handle, className)
    return RealmResults:new(self, result_handle, classInfo)
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
