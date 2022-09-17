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

local persons = realm:objects("Person")

local function onPersonsChange(persons, changes)
    print("Reacting to changes..")

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

-- Collection listener
-- NOTE: Consume the return value in order to not be garbage collected
local notificationToken = persons:addListener(onPersonsChange)
print(notificationToken)


local test_person
realm:write(function()
    test_person = realm:create("Person")
    test_person.name = "Jacob"
    test_person.age = 1337
    return 0
end)
print(test_person["name"])
print(test_person["age"])


print("#persons:", #persons)
if (#persons > 0) then
    -- 1-based indexing
    print("persons[1].name:", persons[1].name)

    -- Object listener
    -- persons[1].addListener(onPersonChange)
end

local filtered_persons = persons:filter("name = $0 and age = $1", "Jacob", 1337)
print("len filter...")
print(#filtered_persons)
