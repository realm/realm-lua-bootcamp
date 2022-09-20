local native = require "_realm_native"
local RealmObject = require "realm.object"

---@class RealmResults
---@field class Realm.Schema.ClassInformation
---@field addListener function
---@field filter function
---@field _handle userdata
---@field _realm Realm
local RealmResults = {}

local function addListener(self, onCollectionChange)
    -- Create a listener that is passed to cpp which, when called, in turn calls
    -- the user's listener (onCollectionChange). This makes it possible to pass
    -- the result (self) from Lua instead of cpp.
    local function listener(changes)
        onCollectionChange(self, changes)
    end
    return native.realm_results_add_listener(self._handle, listener)
end

local function filter(self, query_string, ...)
    local handle = native.realm_results_filter(self._handle, self._realm._handle, self.class.name, query_string, select('#', ...), ...)
    return RealmResults:new(self._realm, handle, self.class.name)
end

function RealmResults:new(realm ,handle, classInfo)
    local result = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
        addListener = addListener,
        filter = filter,
    }
    result = setmetatable(result, RealmResults)
    return result
end

function RealmResults:__index(key)
    local object = {
        _handle = native.realm_results_get(self._handle, key - 1),
        _realm = self._realm,
        class = self.class,
    }
    object = setmetatable(object, RealmObject)
    return object
end

function RealmResults:__len()
    return native.realm_results_count(self._handle)
end

return RealmResults
