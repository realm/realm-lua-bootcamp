local native = require "_realm_native"

---@class Realm
local Realm = {}
Realm.__index = Realm

---@module '.schema'

---@class Realm.Config
---@field path string
---@field schemaVersion integer
---@field schema Realm.Schema.ClassDefinition[]

---@param config Realm.Config
function Realm.open(config)
    local self = setmetatable({
        _handle = native.realm_open(config)
    }, Realm)
    return self
end

local RealmObject = {
    __index = function(mytable, key)
        return native.realm_get_value(mytable._realm._handle, mytable._handle, key)
    end,
    __newindex = function(mytable, key, value)
        native.realm_set_value(mytable._realm._handle, mytable._handle, key, value)
    end
}

function Realm:create(class_name)
    local object = {
        _handle = native.realm_object_create(self._handle, class_name),
        _realm = self
    }
    object = setmetatable(object, RealmObject)
    return object
end

function Realm:begin_transaction()
    native.realm_begin_write(self._handle)
end

function Realm:commit_transaction()
    native.realm_commit_transaction(self._handle)
end

function Realm:cancel_transaction()
    native.realm_cancel_transaction(self._handle)
end

---@generic T
---@param writeCallback fun(): T
---@return T
function Realm:write(writeCallback)
    self:begin_transaction()
    local status, result = pcall(writeCallback)
    if (status) then
        self:commit_transaction()
        return result
    else
        self:cancel_transaction()
        error(result)
    end
end

---@generic T
---@param class `T`
---@param values? T
---@return T

return Realm
