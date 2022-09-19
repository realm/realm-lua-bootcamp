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
    local class_info = self._schema[className]
    if class_info == nil then
        error("Class not found in schema");
        return nil
    end
    return RealmObject:new(self, class_info)
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
    local result_handle = native.realm_object_get_all(self._handle, className)
    return self:_createResults(result_handle, className)
end

function Realm:_createResults(handle, className)
   local result = {
        _handle = handle,
        _realm = self,
    }
    function result:addListener(onCollectionChange)
        -- Create a listener that is passed to cpp which, when called, in turn calls
        -- the user's listener (onCollectionChange). This makes it possible to pass
        -- the result (self) from Lua instead of cpp.
        local function listener(changes)
            onCollectionChange(self, changes)
        end
        return native.realm_results_add_listener(handle, listener)
    end
    function result:filter(query_string, ...)
        local handle = native.realm_results_filter(self._handle, self._realm._handle, className, query_string, select('#', ...), ...)
        return self._realm:_createResults(handle, className)
    end
    result = setmetatable(result, RealmResults)
    return result
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
