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

realm:close()
