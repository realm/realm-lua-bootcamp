assert:register("assertion", "is_realm_handle", function(state, arguments)
    return getmetatable(arguments[1]).__name == "_realm_handle"
end)

local Realm = require "realm"
local LuaFileSystem = require "lfs"

local function makeTempPath()
    local path = LuaFileSystem.currentdir()
    return path .. "/test.realm"
end

---@class Person
---@field name string
---@field age integer

---@type Realm.Schema.ClassDefinition[]
local schema = {
    {
        name = "Person",
        properties = {
            name = "string",
            age = "int",
            pet = "Pet?",
        }
    },
    {
        name = "Pet",
        properties = {
            name = "string",
            category = "string",
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

describe("Realm Lua tests", function()
    ---@type Realm
    local realm
    local path

    before_each(function ()
        path = makeTempPath()
        realm = Realm.open({ path = path, schemaVersion = 0, schema = schema})
    end)
    after_each(function ()
        realm:close()
        os.remove(path)
    end)

    it("should work", function()
        assert.is_realm_handle(realm._handle)
    end)

    describe("Creating objects", function()
        it("should work", function ()
            local obj = realm:write(function() return realm:create("Person", { name = "Peter", age = 3 }) end)
            assert.is_realm_handle(obj._handle)
            assert.are.equal(obj.name, "Peter")
            assert.are.equal(obj.age, 3)
            assert.are.equal(obj.pet, nil)
        end)
    end)

end)