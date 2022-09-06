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

function Realm:begin_transaction()
    native.realm_begin_write(self)
end

function Realm:commit_transaction()
end

function Realm:cancel_transaction()
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
function Realm:create(class, values)
    return values
end

return Realm
