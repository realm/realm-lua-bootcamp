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
        }
    }
})

---@class RealmCollectionChanges
---@field deletions table
---@field insertions table
---@field modifications table
---@field modificationsAfter table

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

    for _, modificationIndex in ipairs(changes.modifications) do
        print("Modification:", "name:", persons[modificationIndex].name, "age:", persons[modificationIndex].age)
    end

    for _, modificationIndexAfter in ipairs(changes.modificationsAfter) do
        print("Modification:", "name:", persons[modificationIndexAfter].name, "age:", persons[modificationIndexAfter].age)
    end
end

---@class RealmObjectChanges
---@field isDeleted boolean
---@field modifiedProperties table

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
