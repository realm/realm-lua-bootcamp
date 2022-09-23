---@module "realm.scheduler"

---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---Externally defined classes
---@see Realm.Object
---@see Realm.Results

---@class Realm.Config
---@field path string
---@field schemaVersion integer
---@field schema Realm.Schema.ClassDefinition[]
---@field scheduler Realm.Scheduler?
---@field sync Realm.Config.Sync?
---@field _cached boolean? return a cached Realm instance, default is true

---@alias Realm.Handle userdata

---@class Realm.ObjectChanges
---@field isDeleted boolean
---@field modifiedProperties table<number, number>  -- NOTE: Will change to table<number, string>

---@alias Realm.ObjectChanges.Callback fun(object: Realm.Object, changes: Realm.ObjectChanges) 

---@class Realm.CollectionChanges
---@field deletions table<number, number>
---@field insertions table<number, number>
---@field modificationsOld table<number, number>
---@field modificationsNew table<number, number>

---@alias Realm.CollectionChanges.Callback fun(results: Realm.Results, changes: Realm.CollectionChanges)

---@class Realm.Config.Sync
---@field user RealmUser The currently logged in user.
---@field partitionValue string The value used for syncing objects with its partition key field set to this value.

---@class Realm.Schema.ClassInformation Schema classes information returned after opening a Realm
---@field name string Class name
---@field key integer Class key
---@field properties table<string, Realm.Schema.PropertyDefinition>
---@field primaryKey string

---@class Realm.Schema.PropertyDefinition
---@field type Realm.Schema.PropertyType
---@field key number? Property key, only defined with properties from ClassInformation
---@field mapTo string?
---@field indexed boolean?
---@field optional boolean?

---@class Realm.Schema.ClassDefinition Schema classes definition used to open a Realm
---@field name string
---@field primaryKey string?
---@field properties table<string, Realm.Schema.PropertyDefinition | Realm.Schema.PropertyType>

---@class Realm.Schema.ClassInformation Schema classes information returned after opening a Realm
---@field key integer Class key
---@field properties table<string, Realm.Schema.PropertyInformation>
---@field primaryKey string?

---@class Realm.Schema.PropertyInformation
---@field key userdata
---@field name string
---@field type PropertyInformation.Type
---@field objectType string?
---@field collectionType PropertyInformation.CollectionType?

---@enum PropertyInformation.Type
local PropertyType = {
    INT = 0,
    BOOL = 1,
    STRING = 2,
    BINARY = 4,
    MIXED = 6,
    TIMESTAMP = 8,
    FLOAT = 9,
    DOUBLE = 10,
    DECIMAL128 = 11,
    OBJECT = 12,
    LINKING_OBJECTS = 14,
    OBJECT_ID = 15,
    UUID = 17,
}

---@enum PropertyInformation.CollectionType
local CollectionType = {
    List = 1,
    Set = 2,
    Dictionary = 4, 
}

return {
    PropertyType = PropertyType,
    CollectionType = CollectionType,
}
