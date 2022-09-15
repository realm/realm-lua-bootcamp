---@meta

---@class RealmObject
RealmObject = {
    __index = function(mytable, key)
        return Native.realm_get_value(mytable._realm._handle, mytable._handle, key)
    end,
    __newindex = function(mytable, key, value)
        Native.realm_set_value(mytable._realm._handle, mytable._handle, key, value)
    end
}
