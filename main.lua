print("Hello, world!")

print(my_custom_sum(1, 2, 3))

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
local p = realm:write(function()
    return realm:create("Person", { name = "Mads", age = 10 })
end)
print(p.name)
