local native = require "_realm_native"

---@class RealmCredentials
---@field _handle userdata The realm credentials userdata.
local RealmCredentials = {}

---Get a RealmCredentials object to use for authenticating an anonymous user.
---@return RealmCredentials
function RealmCredentials:anonymous()
    -- NOTE: Prioritize "emailPassword" over anonymous

    -- Call native.<function> which should call C: realm_app_credentials_new_anonymous

end

---Get a RealmCredentials object to use for authenticating a user with email and password.
---@param email string The email to use.
---@param password string The password to associate with the email.
---@return RealmCredentials
function RealmCredentials:emailPassword(email, password)
    local credentials = {
        _handle = native.realm_app_credentials_new_email_password(email, password)
    }
    credentials = setmetatable(credentials, RealmCredentials)
    return credentials
end

return RealmCredentials
