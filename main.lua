---@diagnostic disable: undefined-field
local uv = require "luv"
local Realm = require "realm"
require "realm.scheduler.libuv"

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
                age = "int",
                pet = "Pet?",
                pets = "Pet[]",
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
})

local function assertRealmHandle(handle, message)
    assert(getmetatable(handle).__name == "_realm_handle", message)
end

-------------------- TESTING REFERENCES FEATURES -------------------- 
print("Testing references...")

realm:write(function()
    local personWithACat = realm:create("Person")
    assert(personWithACat)
    personWithACat.name = "Katy"

    local cat = realm:create("Pet")
    assert(cat)
    cat.name = "Mongo"
    assert(cat.name == "Mongo")

    personWithACat.pet = cat
    assert(personWithACat.pet)
    assert(personWithACat.pet.name == "Mongo", "Referenced object must match reference properties")
    cat.category = "Cat"
    assert(personWithACat.pet.category == "Cat", "Updating referenced object must match reference")

    personWithACat.pet.category = "Dog"
    assert(cat.category == "Dog", "Updating reference must match referenced object")

    realm:delete(personWithACat)
    realm:delete(cat)
end)

-------------------- TESTING FILTER FEATURES -------------------- 
print("Testing filter features...")
local trackedPersons = realm:objects("Person")
local randomAge = math.random(1, 1000)
local filteredPersons = trackedPersons:filter("age = $0", randomAge)
local testPersonA
local testPersonB

realm:write(function ()
    testPersonA = realm:create("Person", {name = "Jacob", age = randomAge})
    testPersonB = realm:create("Person", {name = "Mongo", age = randomAge})
end)

if #filteredPersons ~= 2 then
    error("Only 2 specified persons must be caught by filter, instead got " .. #filteredPersons ..
    ". Please try re-running or deleting your bootcamp.realm file.");
    return
end

local filteredPersonsSpecific = trackedPersons:filter("name = $0 and age = $1", "Jacob", randomAge)
local filteredPerson = filteredPersonsSpecific[1]
realm:write(function()
    --update created person, works due to live objects
    filteredPerson.name = "John"
    filteredPerson.age = 42
    return 0
end)

assert(filteredPerson.name == "John", "Filter changes to name must be updated")
assert(filteredPerson.age == 42, "Filter changes to age must be updated")

realm:write(function()
    realm:delete(testPersonA)
    realm:delete(testPersonB)
    return 0
end)

assert(not realm:isValid(testPersonA), "Filtered object must be properly deleted")
-- -------------------- TESTING PRIMARY KEY FEATURES -------------------- 
print("Testing primary key features...")
realm:write(function()
    testPersonA = realm:create("Person2", {name = "pk3", age = 1337})
    assert(not pcall(function()
        testPersonB = realm:create("Person2", {name = "H" .. randomAge, age = 2000})
    end), "Function must throw error when object with same primary key is created")
    realm:delete(testPersonA)
    return 0
end)

-------------------- TESTING OBJECT CHANGE NOTIFICATIONS -------------------- 
---@type Realm.ObjectChanges
local personObjectChanges
local trackedPerson
local collectionChangeTest

local objectCallbackTester = coroutine.create(function ()
    realm:write(function()
        trackedPerson.name = "Woo"
    end)
    coroutine.yield()

    assert(#personObjectChanges.modifiedProperties == 1, "Only one property must have been changed")
    realm:write(function()
        trackedPerson.name = "Yeah"
        trackedPerson.age = 100
    end)
    coroutine.yield()

    assert(#personObjectChanges.modifiedProperties == 2, "Only two properties must have been changed")
    realm:write(function()
        realm:delete(trackedPerson)
    end)
    coroutine.yield()
    -- Start the collection change test
    collectionChangeTest()
end)

realm:write(function()
    print("Testing object changes...")
    trackedPerson = realm:create("Person", {name = "Jacob", age = 1337})
end)

---@type Realm.ObjectChanges.Callback
local function onPersonChange(person, changes)
    assert(person._handle == trackedPerson._handle, "Object changes must return reference to correct object")
    personObjectChanges = changes
    assert(coroutine.resume(objectCallbackTester))
end

-- NOTE: Consume the return value in order to not be garbage collected
local objectNotificationToken = trackedPerson:addListener(onPersonChange)
assertRealmHandle(objectNotificationToken, "Object notification token must be a Realm Handle")


-------------------- TESTING COLLECTION CHANGE NOTIFICATIONS --------------------
-- Will be run only after the object change test is complete
collectionChangeTest = function()
    local trackedPets = realm:objects("Pet")
    print("Testing collection changes...")
    ---@type Realm.CollectionChanges
    local personCollectionChanges

    local collectionCallbackTester = coroutine.create(function ()
        local newCat;
        local newDog;
        realm:write(function()
            newCat = realm:create("Pet", {category = "Cat"})
            newDog = realm:create("Pet", {name = "Buddy", category = "Dog"})
        end)
        coroutine.yield()

        assert(#personCollectionChanges.insertions == 2, "Only 1 insertion should be reported")
        assert(#personCollectionChanges.modificationsNew == 0, "0 modifications should be reported")
        assert(#personCollectionChanges.deletions == 0, "0 deletions should be reported")

        realm:write(function()
            newCat.name = "Mono"
            realm:delete(newDog)
        end)
        coroutine.yield()

        assert(#personCollectionChanges.insertions == 0, "0 insertion should be reported")
        assert(#personCollectionChanges.modificationsNew == 1, "Only 1 modifications should be reported")
        assert(#personCollectionChanges.deletions == 1, "Only 1 deletion should be reported")
    end)

    ---@see Realm.CollectionChanges.Callback
    local function onPersonsChange(persons, changes)
        personCollectionChanges = changes
        assert(coroutine.resume(collectionCallbackTester))
    end

    local collectionNotificationToken = trackedPets:addListener(onPersonsChange)
    assertRealmHandle(collectionNotificationToken, "Collection notification token must be a Realm Handle")
    print("All tests passed!")
end
