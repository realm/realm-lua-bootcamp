local Realm = require "realm"
local RealmApp = require "realm.app"
local RealmCredentials = require "realm.credentials"

-- EXAMPLE SYNC USAGE: --

local APP_ID = "MY_APP_ID"
local app = RealmApp.new({ appId = APP_ID })
local currentUser = app:currentUser()
local realmSync

local coroutineOpenRealm = coroutine.create(function ()
    realmSync = Realm.open({
        schema = {
            {
                name = "StoreSync",
                properties = {
                    -- Use e.g. "city" as the partition key (configure on backend)
                    city = "string",
                    numEmployees = "int"
                }
            }
        },
        sync = {
            user = currentUser,
            -- Sync all the stores with partition key ("city") set to "Chicago".
            partitionValue = "Chicago"
            -- NOTE: There are more sync config props that could be set.
        }
    })
end)

local function registerAndLogIn(email, password)
    -- When the registration is complete, the callback will be invoked.
    app:registerEmail(email, password, function ()
        -- When the login is complete, the callback will be invoked.
        local credentials = RealmCredentials:emailPassword(email, password)
        app:logIn(credentials, function (user)
            currentUser = user
            coroutine.resume(coroutineOpenRealm)
        end)
    end)
end

-- If there is a current user, it has been authenticated and is logged in,
-- whereafter we resume the coroutine and open the realm. Otherwise we register
-- a new user and log in before opening the realm.
if currentUser then
    coroutine.resume(coroutineOpenRealm)
else
    registerAndLogIn("jane@example.com", "12345")
end


-- EXAMPLE NON-SYNC USAGE: --

---@class Person
---@field name string
---@field age integer

local realm = Realm.open({
    path = "./bootcamp.realm",
    schemaVersion = 0,
    schema = {
        {
            name = "Person",
            properties = {
                name = "string",
                age = "int"
            }
        }
    }
})

---@class RealmCollectionChanges
---@field deletions table<number, number>
---@field insertions table<number, number>
---@field modificationsOld table<number, number>
---@field modificationsNew table<number, number>

---@param persons RealmResults
---@param changes RealmCollectionChanges
local function onPersonsChange(persons, changes)
    print("Reacting to Person collection changes..")

    for _, deletionIndex in ipairs(changes.deletions) do
        print("Deletion:", "index:", deletionIndex)
    end

    for _, insertionIndex in ipairs(changes.insertions) do
        print("Insertion:", "name:", persons[insertionIndex].name, "age:", persons[insertionIndex].age)
    end

    for _, modificationIndexNew in ipairs(changes.modificationsNew) do
        print("Modification:", "name:", persons[modificationIndexNew].name, "age:", persons[modificationIndexNew].age)
    end
end

---@class RealmObjectChanges
---@field isDeleted boolean
---@field modifiedProperties table<number, number>  -- NOTE: Will change to table<number, string>

---@param person RealmObject
---@param changes RealmObjectChanges
local function onPersonChange(person, changes)
    print("Reacting to Person object changes...")

    if (changes.isDeleted) then
        print("The person was deleted.")
        return
    end

    for _, prop in ipairs(changes.modifiedProperties) do
        -- TODO:
        -- We're currently just receiving the prop key as an int (typedef int64_t realm_property_key_t)
        print("prop:", prop)

        -- But we should receive the string property names so that we can write: person[prop]
        -- print("Modification:", "Modified prop:", prop, "New value:", person[prop])

        -- Would be good to have the corresponding names cached on the Lua side so that we don't
        -- need to loop the property keys in CPP and call the C API each time the object changes.
    end
end

local persons = realm:objects("Person")

-- NOTE: Consume the return value in order to not be garbage collected
local personsCollectionNotificationToken = persons:addListener(onPersonsChange)
print("Collection notification token:", personsCollectionNotificationToken)

local testPerson
realm:write(function()
    testPerson = realm:create("Person")
    testPerson.name = "Jacob"
    testPerson.age = 1337
    return 0
end)

-- NOTE: Consume the return value in order to not be garbage collected
local personObjectNotificationToken = testPerson:addListener(onPersonChange)
print("Object notification token:", personObjectNotificationToken)

local filteredPersons = persons:filter("name = $0 and age = $1", "Jacob", 1337)
print("#filteredPersons:", #filteredPersons)

-- TODO:
-- Deal with notification_token and use when closing a realm
