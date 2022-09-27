---@diagnostic disable: undefined-field
local uv = require "luv"
local Realm = require "realm"
require "realm.scheduler.libuv"
local App = require "realm.app"

local signal = uv.new_signal()
signal:start("sigint", function()
  print("\nReceived interrupt signal. Exiting.")
  uv.stop()
end)
signal:unref()

function coroutine.resumeThrowable(co, val1, ...)
    local results = table.pack(coroutine.resume(co, val1, ...))
    if not results[1] then
        error(results[2])
    end

    return table.unpack(results)
end

-- EXAMPLE SYNC USAGE: --

local APP_ID = "application-0-oltdi"
local app = App.new({ appId = APP_ID })
local currentUser = app:currentUser()
local realm --- @type Realm

local coroutineOpenRealm = coroutine.create(function ()
    assert(currentUser)
    realm = Realm.open({
        schema = {
            {
                name = "StoreSync",
                primaryKey = "_id",
                properties = {
                    _id = "int",
                    -- Use e.g. "city" as the partition key (configure on backend)
                    city = "string",
                    numEmployees = "int"
                }
            }
        },
        sync = {
            user = currentUser,
            -- Sync all the stores with partition key ("city") set to "Chicago".
            -- the partition value is the raw Extended JSON string (Lua does not have a BSON package)
            partitionValue = "\"Chicago\""
        }
    })
end)

local function registerAndLogIn(email, password)
    -- When the registration is complete, the callback will be invoked.
    app:registerEmail(email, password, function (err)
        if not err or err == "name already in use" then
            -- When the login is complete, the callback will be invoked.
            local credentials = App.credentials.emailPassword(email, password)
            app:logIn(credentials, function (user, err)
                if not err then
                    currentUser = user
                    coroutine.resumeThrowable(coroutineOpenRealm)
                else
                    error(err)
                end
            end)
        else
            error(err)
        end
    end)
end

-- If there is a current user, it has been authenticated and is logged in,
-- whereafter we resume the coroutine and open the realm. Otherwise we register
-- a new user and log in before opening the realm.
if currentUser then
    coroutine.resumeThrowable(coroutineOpenRealm)
else
    registerAndLogIn("jane@example.com", "123456")
end

local stores = realm:objects("StoreSync")
print("Listening for Store collection notifications...")
local storesToken = stores:addListener(function (collection, changes)
    for _, index in ipairs(changes.deletions) do
        print("Deleted store at index " .. index)
    end

    for _, index in ipairs(changes.insertions) do
        print("Added store with id " .. collection[index]._id)
    end

    for _, index in ipairs(changes.modificationsNew) do
        print("Modified store with id " .. collection[index]._id)
    end
end)

realm:write(function()
    -- Create objects that should sync (city = "Chicago")
    local storeA = realm:create("StoreSync", {
        _id = math.random(1, 100000),
        city = "Chicago",
        numEmployees = 10
    })
    assert(storeA)
    assert(storeA.city == "Chicago", "'city' property does not match expected.")
    assert(storeA.numEmployees == 10, "'numEmployees' property does not match expected.")

    local storeB = realm:create("StoreSync", {
        _id = math.random(1, 100000),
        city = "Chicago",
        numEmployees = 20
    })
    assert(storeB)
    assert(storeB.city == "Chicago", "'city' property does not match expected.")
    assert(storeB.numEmployees == 20, "'numEmployees' property does not match expected.")
end)

local firstStore = stores[1]
local firstStoreId = firstStore._id
print("Listening for object notifications on Store with id " .. firstStoreId .. "...")
local firstStoreToken = firstStore:addListener(function (object, changes)
    if changes.isDeleted then
        print("Deleted store with id " .. firstStoreId)
    elseif #changes.modifiedProperties > 0 then
        print("Modified store with id " .. object._id)
    end
end)

print("Press Ctrl+C to exit")
uv.run()
