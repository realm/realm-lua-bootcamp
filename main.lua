---@diagnostic disable: undefined-field

--------------------------------------------------------------------------------
-- EXAMPLE USAGE OF REALM LUA USING A LOCAL (NON-SYNC) REALM
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Import Realm
--------------------------------------------------------------------------------

local uv = require "luv"

local Realm = require "realm"
require "realm.scheduler.libuv"

local signal = uv.new_signal()
signal:start("sigint", function()
  print("\nReceived interrupt signal. Exiting.")
  uv.stop()
end)
signal:unref()

--------------------------------------------------------------------------------
-- Define Your Object Model
--------------------------------------------------------------------------------

local TeamSchema = {
    name = "Team",
    primaryKey = "_id",
    properties = {
        _id = "int",
        teamName = "string",
        tasks = "Task[]"
    }
}

local TaskSchema = {
    name = "Task",
    primaryKey = "_id",
    properties = {
        _id = "int",
        description = "string",
        completed = "bool",
        size = "string?"
    }
}

-----------------------------------------------------------------------------
-- Open a Local (Non-Sync) Realm
-----------------------------------------------------------------------------

local realm = Realm.open({
    schema = { TeamSchema, TaskSchema }
})

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
    description = "Get started with Realm Lua",
    completed = false,
    size = size.SMALL,
}
local mediumTask = {
    _id = math.random(1, 100000),
    description = "Build an app using Atlas Device Sync",
    completed = false,
    size = size.MEDIUM
}
local luaTeam = {
    _id = math.random(1, 100000),
    teamName = "Lua",
}

-- Before the write transaction, `smallTask`, `mediumTask`
-- and `luaTeam` are only regular Lua objects.

realm:write(function ()
    -- `realm:create()` returns the created Realm
    -- object, so we can assign it to our variables.

    smallTask = realm:create("Task", smallTask)
    mediumTask = realm:create("Task", mediumTask)
    luaTeam = realm:create("Team", luaTeam)
    table.insert(luaTeam.tasks, smallTask)
    table.insert(luaTeam.tasks, mediumTask)

    assert(smallTask and smallTask.description == "Get started with Realm Lua")
    assert(mediumTask and mediumTask.description == "Build an app using Atlas Device Sync")
    assert(luaTeam and luaTeam.teamName == "Lua")
    assert(#luaTeam.tasks == 2)
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
    description = "Build a great IoT app",
    completed = false,
    size = size.LARGE
}

realm:write(function ()
    -- Modify `smallTask`
    smallTask.completed = true

    -- Modify `largeTask`
    largeTask = realm:create("Task", largeTask)
    table.insert(luaTeam.tasks, largeTask)
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
    description = "Insert data and watch it notify you",
    completed = false,
    size = size.SMALL
}

-- (1) Trigger insertion notification:
realm:write(function ()
    insertedTask = realm:create("Task", insertedTask)
end)

-- (2) Trigger modification notification:
realm:write(function ()
    insertedTask.description = "Modify data and watch it notify you"
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
    smallTask.description = "Modify the small task"
end)

-- (2) Trigger deletion notification:
realm:write(function ()
    realm:delete(smallTask)
end)

-- NOTE:
-- * If the object we added the listener to is also in the collection
--   that we added a listener to, both listeners will be called.


print("Press Ctrl+C to exit")
uv.run()
