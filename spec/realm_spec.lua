---Disable undefined globals to account for globals
---provided by the testing library
---@diagnostic disable: undefined-global

---This forces the Lua language server to override
---the built-in definition of assert.
assert = assert

local uv = require "luv"
local Realm = require "realm"
require "realm.scheduler.libuv"
local LuaFileSystem = require "lfs"
local inspect = require "inspect"

assert:register("assertion", "is_realm_handle", function(_, arguments)
    return getmetatable(arguments[1]).__name == "_realm_handle"
end)

local function makeTempPath()
    local path = LuaFileSystem.currentdir()
    return path .. "/test.realm"
end

---@param milliseconds integer
local function timeout(milliseconds)
    local timer = uv.new_timer()
    timer:start(milliseconds, 0, function ()
        error("timeout")
        timer:stop()
        timer:close()
    end)
    return timer
end

local function post(cb)
    local async
    async = uv.new_async(function ()
        cb()
        async:close()
    end)
    async:send()
    return async
end

---Helper function for cleaning up created objects
---@param realm Realm
---@param objects Realm.Object
local function _delete(realm, objects)
    realm:write(function()
        for _, object in ipairs(objects) do
            realm:delete(object)
        end
    end)
end

---@class Pet:Realm.Object
---@field name string
---@field category string

---@class Person:Realm.Object
---@field name string
---@field age integer
---@field pet Pet

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
        name = "PersonWithPK",
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
    ---@type Person
    local testPerson

    setup(function()
        path = makeTempPath()
        realm = Realm.open({ path = path, schemaVersion = 0, schema = schema})
        realm:write(function()
            testPerson = realm:create("Person", { name = "Peter", age = 3 })
        end)
    end)
    teardown(function()
        realm:write(function()
            realm:delete(testPerson)
        end)
        realm:close()
        os.remove(path)
    end)

    describe("Creating and modifying objects", function()
        it("should set proper Realm handle at Realm open", function()
            assert.is_realm_handle(realm._handle)
        end)
        it("should set correct properties at creation", function()
            assert.is_realm_handle(testPerson._handle)
            assert.are.equal(testPerson.name, "Peter")
            assert.are.equal(testPerson.age, 3)
            assert.are.equal(testPerson.pet, nil)
        end)
        it("should correctly change properties", function()
            realm:write(function()
                testPerson.name = "Aram"
                testPerson.age = 100
            end)
            assert.are.equal(testPerson.name, "Aram")
            assert.are.equal(testPerson.age, 100)
            assert.are.equal(testPerson.pet, nil)
        end)
        describe("with references fields", function()
            local testPet
            setup(function()
                realm:write(function() 
                    testPet = realm:create("Pet", { name = "Oreo", category = "Cat" })
                    testPerson.pet = testPet
                end)
            
            end)
            teardown(function() _delete(realm, {testPet}) end)

            it("should set them correctly", function()
                assert.is_realm_handle(testPerson.pet._handle)
                assert.are.equal(testPerson.pet.name, testPet.name)
                assert.are.equal(testPerson.pet.category, testPet.category)
            end)
            it("should update in sync with references", function()
                realm:write(function()
                    testPet.category = "Dog"
                    testPet.name = "Thor"
                end)
                assert.are.equal(testPerson.pet.name, testPet.name)
                assert.are.equal(testPerson.pet.category, testPet.category)
            end)
        end)
        describe("with classes that have a primary key", function()
            local testPersonPK
            setup(function()
                realm:write(function() 
                    testPersonPK = realm:create("PersonWithPK", { name = "Unique", age = 25 })
                end)
            end)
            teardown(function() _delete(realm, {testPersonPK}) end)

            it("should succesfuly create and set properties", function()
                assert.is_realm_handle(testPersonPK._handle)
            end)
            it("should not let user create objects without a primary key", function()
                realm:write(function()
                    local badCreateA = function() realm:create("PersonWithPK") end
                    local badCreateB = function() realm:create("PersonWithPK", { age = 10 }) end
                    assert.has_error(badCreateA, "Primary key not set at declaration")
                    assert.has_error(badCreateB, "Primary key not set at declaration")
                end)
            end)
            it("should not let user create duplicate objects with the same primary key", function()
                realm:write(function()
                    local badCreate = function() realm:create("PersonWithPK", { name = "Unique" }) end
                    assert.has_error(badCreate, "Object with this primary key already exists")
                end)
            end)
        end)
        describe("with notifications", function()
            local notificationReceived = false;

            local people = realm:objects("Person")
            local notificationToken = people:addListener(function (collection, changes)
                if #changes.insertions == 1 then
                    notificationReceived = true;
                    uv.stop()
                end
            end)
            assert.is_realm_handle(notificationToken)

            post(function ()
                local otherRealm <close> = Realm.open({path = path, schemaVersion = 0, schema = schema, _cached = false})
                otherRealm:write(function()
                    otherRealm:create("Person")
                end)
            end)

            timeout(1000)

            uv.run()

            assert.True(notificationReceived)
        end)
    end)
end)
