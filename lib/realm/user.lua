local native = require "_realm_native"

---@class RealmUser
---@field id string The id of the user.
---@field _handle userdata The realm user userdata.
local RealmUser = {}

---@param handle userdata The realm user userdata.
---@return RealmUser | nil
function RealmUser._new(handle)
    if handle == nil then
        return nil
    end

    local user = {
        _handle = handle,
        id = native.realm_user_get_id(handle)
    }
    user = setmetatable(user, RealmUser)
    return user
end

---Log out the user by removing its credentials.
function RealmUser:logOut()
    native.realm_user_log_out(self._handle)
end

return RealmUser
