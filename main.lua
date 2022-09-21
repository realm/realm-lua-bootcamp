local Realm = require "realm"

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
        },
        {
            name = "Person2",
            properties = {
                name = "string",
                age = "int"
            },
            primaryKey = "name"
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
    testPerson = realm:create("Person", {name = "Jacob", age = 1337})
    return 0
end)

-- NOTE: Consume the return value in order to not be garbage collected
local personObjectNotificationToken = testPerson:addListener(onPersonChange)
print("Object notification token:", personObjectNotificationToken)

local filteredPersons = persons:filter("name = $0 and age = $1", "Jacob", 1337)
print("#filteredPersons:", #filteredPersons)

local filteredPerson = filteredPersons[1]
realm:write(function()
    --update created person, works due to live objects
    filteredPerson.name = "John"
    filteredPerson.age = 42
    return 0
end)
print("filteredperson name", filteredPerson.name)
print("filteredperson age:", filteredPerson.age)

realm:write(function()
    realm:delete(testPerson)
    return 0
end)

if not realm:isValid(testPerson) then
    print("Object was successfully deleted")
end

-- NOTE: Uncomment to test PK functionality, could be annoying to have since it will throw an error if run multiple times
-- local testPerson2
-- realm:write(function()
--     testPerson2 = realm:create("Person2", {name = "pk9", age = 1337})
--     return 0
-- end)
-- print("testPerson2 name: ", testPerson2.name)

-- TODO:
-- Deal with notification_token and use when closing a realm
