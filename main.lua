local Realm = require "realm"

---@class Person
---@field name string
---@field age integer

local realm = Realm.open({
    path = os.getenv( "HOME" ) .. "/Documents/realm-lua-bootcamp/build/bootcamp.realm",
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

print(realm._handle, "\n")

local p = realm:write(function()
    return realm:create("Person", { name = "Mads", age = 10 })
end)
print(p.name)
