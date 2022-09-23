local native = require "realm.native"
local RealmUser = require "realm.user"

---@class RealmAppConfiguration
---@field appId string The unique ID for the Atlas App Services application.
--- NOTE: There are more possible fields on RealmAppConfiguration that can be customized by the user.

---@class RealmApp
---@field _handle userdata The realm app userdata.
local RealmApp = {}
RealmApp.__index = RealmApp

---Get the currently logged in user.
---@return RealmUser | nil
function RealmApp:currentUser()
    local userHandle = native.realm_app_get_current_user(self._handle)
    return RealmUser._new(userHandle)
end

---Create a new RealmApp object.
---@param config RealmAppConfiguration The configuration for creating the app.
---@return RealmApp
function RealmApp.new(config)
    local app = {
        _handle = native.realm_app_create(config.appId)
    }
    app = setmetatable(app, RealmApp)
    return app
end

---Register a new email identity.
---@param email string The email to register.
---@param password string The password to associate with the email.
---@param onRegistered fun() The callback to be invoked when registered.
function RealmApp:registerEmail(email, password, onRegistered)
    native.realm_app_register_email(self._handle, email, password, onRegistered)
end

---Log in a user.
---@param credentials RealmCredentials The credentials to use.
---@param onLoggedIn fun(user: RealmUser | nil, error: any?) The callback to be invoked when logged in.
function RealmApp:logIn(credentials, onLoggedIn)
    ---@param userHandle userdata The realm user userdata.
    local function callback(userHandle)
        onLoggedIn(RealmUser._new(userHandle))
    end
    native.realm_app_log_in(self._handle, credentials._handle, callback)

    -- TODO: Handle callback param "error".
end

return RealmApp
