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


local test_person
realm:write(function()
    test_person = realm:create("Person")
    test_person.name = "Jacob"
    test_person.age = 1337
    return 0
end)
print(test_person["name"])
print(test_person["age"])


local persons = realm:objects("Person")

function on_persons_change(persons, changes)
    -- TODO: React to changes
    print("Reacting to changes..")
    print(persons, changes)
end
-- Collection listener
-- NOTE: Consume the return value in order to not be garbage collected
local notification_token = persons.add_listener(on_persons_change)
print(notification_token)

print("#persons:", #persons)
if (#persons > 0) then
    -- 1-based indexing
    print("persons[1].name:", persons[1].name)

    -- Object listener
    -- persons[1].add_listener(on_person_change)
end
