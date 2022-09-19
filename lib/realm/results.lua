local native = require "_realm_native"
local RealmObject = require "realm.object"

-- TODO:
-- Perhaps add RealmResultsBase
-- Set it as RealmResults' metatable
-- Add "filter" etc. as instance methods on RealmResults

---@class RealmResults
---@field addListener function
---@field filter function
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

return RealmResults
