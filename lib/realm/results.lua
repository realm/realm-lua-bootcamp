local native = require "_realm_native"
local RealmObject = require "realm.object"

---@class Realm.Results
---@field class Realm.Schema.ClassInformation
---@field addListener fun(self: Realm.Results, cb: Realm.CollectionChanges.Callback) : Realm.Handle
---@field filter function
---@field _handle userdata
---@field _realm Realm
local RealmResults = {}

---@param self Realm.Results
---@param onCollectionChange Realm.CollectionChanges.Callback
---@return Realm.Handle Notification token
local function addListener(self, onCollectionChange)
    -- Create a listener that is passed to cpp which, when called, in turn calls
    -- the user's listener (onCollectionChange). This makes it possible to pass
    -- the result (self) from Lua instead of cpp.
    local function listener(changes)
        onCollectionChange(self, changes)
    end
    local notificationToken = native.realm_results_add_listener(self._handle, listener)
    table.insert(self._realm._childHandles, notificationToken)
    return notificationToken
end

---@param self Realm.Results
---@param queryString string
---@return Realm.Results
local function filter(self, queryString, ...)
    local handle = native.realm_results_filter(self._handle, self._realm._handle, self.class.name, queryString, select('#', ...), ...)
    return RealmResults:new(self._realm, handle, self.class)
end

---@param realm Realm
---@param handle userdata
---@param classInfo Realm.Schema.ClassInformation
---@return Realm.Results
function RealmResults:new(realm ,handle, classInfo)
    local results = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
        addListener = addListener,
        filter = filter,
    }
    table.insert(realm._childHandles, results._handle)
    results = setmetatable(results, RealmResults)
    return results
end

function RealmResults:__index(key)
    local objectHandle = native.realm_results_get(self._handle, key - 1)
    return RealmObject:new(self._realm, self.class, nil, objectHandle)
end

function RealmResults:__len()
    return native.realm_results_count(self._handle)
end

return RealmResults
