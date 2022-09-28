local native = require "realm.native"
local RealmObject = require "realm.object"

---@class Realm.Results
---@field class Realm.Schema.ClassInformation The class information.
---@field addListener fun(self: Realm.Results, cb: Realm.CollectionChanges.Callback) : Realm.Handle Add a listener to listen to change notifications.
---@field filter function Filter objects from the results.
---@field _handle userdata The realm results userdata.
---@field _realm Realm The realm.
local RealmResults = {}

---@param self Realm.Results The realm results.
---@param onCollectionChange Realm.CollectionChanges.Callback The callback to be notified on changes.
---@return Realm.Handle
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

---@param self Realm.Results The realm results.
---@param queryString string The query string.
---@return Realm.Results
local function filter(self, queryString, ...)
    local handle = native.realm_results_filter(self._handle, self._realm._handle, self.class.name, queryString, select('#', ...), ...)

    return RealmResults._new(self._realm, handle, self.class)
end

---@param realm Realm The realm.
---@param handle userdata The realm results userdata.
---@param classInfo Realm.Schema.ClassInformation The class information.
---@return Realm.Results
function RealmResults._new(realm ,handle, classInfo)
    local results = {
        _handle = handle,
        _realm = realm,
        class = classInfo,
        addListener = addListener,
        filter = filter,
    }
    table.insert(realm._childHandles, results._handle)

    return setmetatable(results, RealmResults)
end

---@param index number The index of the object to get.
---@return Realm.Object
function RealmResults:__index(index)
    local objectHandle = native.realm_results_get(self._handle, index - 1)

    return RealmObject._new(self._realm, self.class, nil, objectHandle)
end

---@return number
function RealmResults:__len()
    return native.realm_results_count(self._handle)
end

return RealmResults
