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
        _handle = native.realm_open(config),
        _childHandles = setmetatable({}, { __mode = "v"}) -- a table of weak references
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
    table.insert(self._childHandles, object._handle)
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

---Explicitly close this realm, releasing its native resources
function Realm:close()
    for _, handle in ipairs(self._childHandles) do
        native.realm_release(handle)
    end
    native.realm_release(self._handle)
end

function Realm.__gc(realm)
    realm:close()
end

-- TODO:
-- Add RealmResultsBase
-- Set it as RealmResults' metatable
-- Add "filter" etc. as instance methods on RealmResults

-- TODO: Add to RealmResults
---@class RealmResults
---@field add_listener function
local RealmResults = {
    __index = function(mytable, key)
        local object = {
            _handle = native.realm_results_get(mytable._handle, key - 1),
            _realm = mytable._realm
        }
        object = setmetatable(object, RealmObject)
        return object
    end,
    __len = function(mytable)
        return native.realm_results_count(mytable._handle)
    end
}

---@param collection_name string
---@return RealmResults
function Realm:objects(collection_name)
    local result_handle = native.realm_object_get_all(self._handle, collection_name)
    local result = {
        _handle = result_handle,
        _realm = self,
        -- NOTE: Preferably move following keys closer to/on RealmResults to prevent
        --       duplicating this code in other functions that return RealmResults like
        --       "filter", "sort", etc.
        add_listener = function(on_collection_change)
            native.realm_results_add_listener(result_handle, on_collection_change)
        end
    }
    result = setmetatable(result, RealmResults)
    return result
end

return Realm
