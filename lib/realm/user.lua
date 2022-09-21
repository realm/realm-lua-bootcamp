---@class RealmUser
---@field id string
---@field _app RealmApp
local RealmUser = {}

---Log out the user by removing its credentials.
function RealmUser:logOut()

    -- Call native.realm_user_log_out which should call C: realm_user_log_out

end

-- The id field on the user should:
--     Call native.realm_user_get_id which should call C: realm_user_get_identity

return RealmUser
