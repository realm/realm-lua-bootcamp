---@diagnostic disable: undefined-field

--------------------------------------------------------------------------------
-- EXAMPLE USAGE OF REALM LUA USING ATLAS DEVICE SYNC
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Import Realm and App
--------------------------------------------------------------------------------

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

--------------------------------------------------------------------------------
-- Initialize Your App Using the App ID Copied From the Atlas App Services UI
--------------------------------------------------------------------------------

local APP_ID = "application-0-oltdi"       -- <--- TODO: INSERT YOUR APP ID <---
local app = App.new({ appId = APP_ID })

--------------------------------------------------------------------------------
-- Define Your Object Model
--------------------------------------------------------------------------------

local TeamSchema = {
    name = "Team",
    primaryKey = "_id",
    properties = {
        _id = "int",
        -- The `_partition` field will store the team name:
        _partition = "string",
        teamName = "string",
        tasks = "Task[]"
    }
}

local TaskSchema = {
    name = "Task",
    primaryKey = "_id",
    properties = {
        _id = "int",
        -- The `_partition` field will store the team name:
        _partition = "string",
        description = "string",
        completed = "bool",
        size = "string?",
        assigneeId = "string?"
    }
}

-----------------------------------------------------------------------------
-- Authenticate a User and Open a Synced Realm
-----------------------------------------------------------------------------

local realm --- @type Realm
local currentUser --- @type Realm.App.User | nil
local partitionValue = "Lua Sync"

---@param user Realm.App.User The realm user.
---@return Realm
local function openRealm(user)
    assert(user)
    return Realm.open({
        schema = { TeamSchema, TaskSchema },
        sync = {
            user = user,
            -- To sync all objects belonging to a team called "Lua Sync" 
            -- (i.e. all objects with the "_partition" field set to "Lua
            -- Sync"), we add it as the partition value. (We will soon
            -- create a team with this name.) The value must be the raw
            -- Extended JSON string in order to be compatible with documents
            -- stored in MongoDB Atlas. (Manually add `"` around the string.)
            partitionValue = "\"" .. partitionValue .. "\""
        }
    })
end

local function registerAndLogIn(email, password)
    app:registerEmail(email, password, function (err)
        -- Called when registration is completed..

        if not err or err == "name already in use" then
            local credentials = App.credentials.emailPassword(email, password)
            app:logIn(credentials, function (user, err)
                -- Called when login is completed..

                if not err then
                    currentUser = user
                    realm = openRealm(user)
                else
                    error(err)
                end
            end)
        else
            error(err)
        end
    end)
end

-- The current App User can also be retrieved once logged in using:
-- local currentUser = app:currentUser()

local exampleEmail = "jane@example.com"
local examplePassword = "123456"

-- If there is a current user, it must have been authenticated and
-- is logged in, whereafter we open the realm. Otherwise we first
-- register a new user, log in, then open the realm.
if currentUser then
    realm = openRealm(currentUser)
else
    registerAndLogIn(exampleEmail, examplePassword)
end

-----------------------------------------------------------------------------
-- Create Realm Objects
-----------------------------------------------------------------------------

local size = {
    SMALL = "SMALL",
    MEDIUM = "MEDIUM",
    LARGE = "LARGE"
}
local smallTask = {
    _id = math.random(1, 100000),
    _partition = partitionValue,
    description = "Get started with Realm Lua",
    completed = false,
    size = size.SMALL,
    assigneeId = currentUser.identity
}
local mediumTask = {
    _id = math.random(1, 100000),
    _partition = partitionValue,
    description = "Build an app using Atlas Device Sync",
    completed = false,
    size = size.MEDIUM
}
local luaSyncTeam = {
    _id = math.random(1, 100000),
    _partition = partitionValue,
    teamName = "Lua Sync",
}

-- Before the write transaction, `smallTask`, `mediumTask`
-- and `luaSyncTeam` are only regular Lua objects.

realm:write(function ()
    -- `realm:create()` returns the created Realm
    -- object, so we can assign it to our variables.

    smallTask = realm:create("Task", smallTask)
    mediumTask = realm:create("Task", mediumTask)
    luaSyncTeam = realm:create("Team", luaSyncTeam)
    table.insert(luaSyncTeam.tasks, smallTask)
    table.insert(luaSyncTeam.tasks, mediumTask)

    assert(smallTask and smallTask.description == "Get started with Realm Lua")
    assert(mediumTask and mediumTask.description == "Build an app using Atlas Device Sync")
    assert(luaSyncTeam and luaSyncTeam.teamName == "Lua Sync")
    assert(#luaSyncTeam.tasks == 2)
end)

-- After the write transaction, the same local
-- variables can now be used as Realm objects.

-----------------------------------------------------------------------------
-- Query Realm Objects
-----------------------------------------------------------------------------

local tasks = realm:objects("Task");

-----------------------------------------------------------------------------
-- Filter Realm Objects
-----------------------------------------------------------------------------

local uncompletedSmallTasks = tasks:filter(
    "completed = $0 AND size = $1",
    false,      -- Replaces $0
    size.SMALL  -- Replaces $1
)

print("Number of uncompleted small tasks: " .. #uncompletedSmallTasks)
print(uncompletedSmallTasks[1].description)

-----------------------------------------------------------------------------
-- Update Realm Objects
-----------------------------------------------------------------------------

local largeTask = {
    _id = math.random(1, 100000),
    _partition = partitionValue,
    description = "Build a great IoT app",
    completed = false,
    size = size.LARGE
}

realm:write(function ()
    -- Modify `smallTask`
    smallTask.completed = true

    -- Modify `largeTask`
    largeTask = realm:create("Task", largeTask)
    table.insert(luaSyncTeam.tasks, largeTask)
end)

print("Number of uncompleted small tasks: " .. #uncompletedSmallTasks)

-----------------------------------------------------------------------------
-- Delete Realm Objects
-----------------------------------------------------------------------------

realm:write(function ()
    realm:delete(largeTask)
    largeTask = nil
end)

-----------------------------------------------------------------------------
-- Get Notified of Changes: Collection Changes
-----------------------------------------------------------------------------

local onTaskCollectionChange = function (collection, changes)
    -- Handle deletions first
    for _, deletedIndex in ipairs(changes.deletions) do
        print("Deleted task at index " .. deletedIndex)
    end

    for _, insertedIndex in ipairs(changes.insertions) do
        print("Added task: " .. collection[insertedIndex].description)
    end

    for _, modifiedIndex in ipairs(changes.modificationsNew) do
        print("Modified task: " .. collection[modifiedIndex].description)
    end
end

-- Add the listener (you currently need to save the return value to a variable)
local _ = tasks:addListener(onTaskCollectionChange)
print("Listening for changes on Task collection...")

-----------------------------------------------------------------------------
-- Get Notified of Changes: Object Changes
-----------------------------------------------------------------------------

local onTaskObjectChange = function (object, changes)
    if changes.isDeleted then
        print("Deleted a task")
    elseif #changes.modifiedProperties > 0 then
        print("Modified task: " .. object.description)
    end
end

-- Add the listener (you currently need to save the return value to a variable)
local _ = smallTask:addListener(onTaskObjectChange)
print("Listening for changes on Task object with id " .. smallTask._id .. "...")

-----------------------------------------------------------------------------
-- Trigger Collection Change Notification
-----------------------------------------------------------------------------

print("\n-------- TRIGGER CHANGE NOTIFICATIONS --------\n")

local insertedTask = {
    _id = math.random(1, 100000),
    _partition = partitionValue,
    description = "Insert data and watch it sync across all devices",
    completed = false,
    size = size.SMALL
}

-- (1) Trigger insertion notification:
realm:write(function ()
    insertedTask = realm:create("Task", insertedTask)
end)

-- (2) Trigger modification notification:
realm:write(function ()
    insertedTask.description = "Modify data and watch it sync across all devices"
end)

-- (3) Trigger deletion notification:
realm:write(function ()
    realm:delete(insertedTask)
end)

-----------------------------------------------------------------------------
-- Trigger Object Change Notification
-----------------------------------------------------------------------------

-- Since we added the listener to `smallTask`, that's the
-- object we need to change to trigger the notification.

-- (1) Trigger modification notification:
realm:write(function ()
    smallTask.assigneeId = currentUser.identity
end)

-- (2) Trigger deletion notification:
realm:write(function ()
    realm:delete(smallTask)
end)

-- NOTE:
-- * If the object we added the listener to is also in the collection
--   that we added a listener to, both listeners will be called.


-- TIP:
-- * While this Lua app is running and you're connected, go to MongoDB Atlas
--   and modify or delete a document directly from the UI. If the document has
--   the same partition value as the one specified when opening the realm, you
--   should see the printouts in the terminal where you're running this code.


print("Press Ctrl+C to exit")
uv.run()
