---@class RealmCredentials
local RealmCredentials = {}

---Get a RealmCredentials object to use for authenticating an anonymous user.
---@return RealmCredentials
function RealmCredentials:anonymous()
    -- NOTE: Prioritize "emailPassword" over anonymous

    -- Call native.<function> which should call C: realm_app_credentials_new_anonymous

end

---Get a RealmCredentials object to use for authenticating a user with email and password.
---@param email string
---@param password string
---@return RealmCredentials
function RealmCredentials:emailPassword(email, password)

    -- Call native.realm_app_credentials_new_email_password which should call C: realm_app_credentials_new_email_password

end

return RealmCredentials
