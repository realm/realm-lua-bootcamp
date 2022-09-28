local native = require "realm.app.native"

---@alias Realm.App.Credentials userdata

local credentials = {}

---Get a credentials object to use for authenticating an anonymous user.
---@param reuseCredentials boolean? Whether to reuse credentials.
---@return Realm.App.Credentials
function credentials.anonymous(reuseCredentials)
    return native.realm_app_credentials_new_anonymous(reuseCredentials);
end

---Get a credentials object to use for authenticating a user with email and password.
---@param email string The email to use.
---@param password string The password to associate with the email.
---@return Realm.App.Credentials
function credentials.emailPassword(email, password)
    return native.realm_app_credentials_new_email_password(email, password)
end

return credentials
