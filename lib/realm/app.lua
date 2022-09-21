---@class RealmAppConfiguration
---@field appId string The unique ID for the Atlas App Services application.
--- NOTE: There are more possible fields on RealmAppConfiguration that can be customized by the user.

---@class RealmApp
---@field currentUser RealmUser The currently logged in user.
local RealmApp = {}

---Create a new RealmApp object.
---@param config RealmAppConfiguration The configuration for creating the app.
---@return RealmApp The app.
function RealmApp:new(config)

    -- Call native.realm_app_create which should call C: realm_app_create

end

---Register a new email identity.
---@param email string The email to register.
---@param password string The password to associate with the email.
---@return Promise<RealmUser> --- TODO: Add Lua's version of Promise, and RealmUser
function RealmApp:registerEmail(email, password)

    -- Call native.realm_app_register_email which should call C: realm_app_email_password_provider_client_register_email

end

---Log in a user.
---@param credentials RealmCredentials The credentials to use.
---@return Promise<RealmUser> --- TODO: Add Lua's version of Promise, and RealmUser
function RealmApp:logIn(credentials)

    -- Call native.realm_app_log_in which should call C: realm_app_log_in_with_credentials

end

return RealmApp
