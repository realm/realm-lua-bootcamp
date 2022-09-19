local native = require "_realm_native"

---@class RealmObject
---@field addListener function
local RealmObject = {
    __index = function(mytable, key)
        return native.realm_get_value(mytable._realm._handle, mytable._handle, key)
    end,
    __newindex = function(mytable, key, value)
        native.realm_set_value(mytable._realm._handle, mytable._handle, key, value)
    end
}

return RealmObject
