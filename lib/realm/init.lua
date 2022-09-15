local native = require "_realm_native"

---@class Realm
---@field _handle userdata
---@field _childHandles userdata[]
local Realm = {}
Realm.__index = Realm

---@module '.schema'
---@module '.object'

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

---@param class_name string
---@return RealmObject
function Realm:create(class_name)
    local object = {
        _handle = native.realm_object_create(self._handle, class_name),
        _realm = self
    }
    table.insert(self._childHandles, object._handle)
    object = setmetatable(object, RealmObject)
    return object
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

---@param className string
---@return RealmResults
function Realm:objects(className)
    local result_handle = native.realm_object_get_all(self._handle, className)
    return self:_createResults(result_handle, className)
end

function Realm:_createResults(handle, className)
   local result = {
        _handle = handle,
        _realm = self,
    }
    function result:add_listener(on_collection_change)
        return native.realm_results_add_listener(handle, on_collection_change)
    end
    function result:filter(query_string, ...)
        local handle = native.realm_results_filter(self._handle, self._realm._handle, className, query_string, select('#', ...), ...)
        return self._realm:_createResults(handle, className)
    end
    result = setmetatable(result, RealmResults)
    return result
end

return Realm
