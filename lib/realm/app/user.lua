local native = require "realm.app.user.native"

---@class Realm.App.User
---@field identity string The server identity of the user.
---@field _handle userdata The realm user userdata.
local User = {}
User.__index = User

---Log out the user by removing its credentials.
function User:logOut()
    native.realm_user_log_out(self._handle)
end

local module = {}

---@param handle userdata The realm user userdata.
---@return Realm.App.User?
function module._new(handle)
    if handle == nil then
        return nil
    end

    local user = {
        _handle = handle,
        identity = native.realm_user_get_identity(handle)
    }

    return setmetatable(user, User)
end

return module
