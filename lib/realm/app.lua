local native = require "realm.app.native"
local RealmUser = require "realm.app.user"

---@class Realm.App.Configuration
---@field appId string The unique ID for the Atlas App Services application.
--- NOTE: There are more possible fields on Realm.App.Configuration that can be customized by the user.

---@class Realm.App
---@field _handle userdata The realm app userdata.
local App = {}
App.__index = App

---Get the currently logged in user.
---@return Realm.App.User?
function App:currentUser()
    local userHandle = native.realm_app_get_current_user(self._handle)

    return RealmUser._new(userHandle)
end

---Register a new email identity.
---@param email string The email to register.
---@param password string The password to associate with the email.
---@param onRegistered fun(error: any?) The callback to be invoked when registered.
function App:registerEmail(email, password, onRegistered)
    native.realm_app_email_password_provider_client_register_email(self._handle, email, password, onRegistered)
end

---Log in a user.
---@param credentials Realm.App.Credentials The credentials to use.
---@param onLoggedIn fun(user: Realm.App.User | nil, error: any?) The callback to be invoked when logged in.
function App:logIn(credentials, onLoggedIn)
    ---@param userHandle userdata The realm user userdata.
    local function callback(userHandle, error)
        onLoggedIn(RealmUser._new(userHandle), error)
    end
    native.realm_app_log_in_with_credentials(self._handle, credentials, callback)
end

local module = {}

---@param config Realm.App.Configuration The configuration for creating the app.
---@return Realm.App
function module.new(config)
    local app = {
        _handle = native.realm_app_create(config.appId)
    }

    return setmetatable(app, App)
end

module.credentials = require "realm.app.credentials"

return module
