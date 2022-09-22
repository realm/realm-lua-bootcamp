---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---@class Realm.Config
---@field path string
---@field schemaVersion integer
---@field schema Realm.Schema.ClassDefinition[]

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

-- Types for the realm schema cache

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
    Int = 0,
    Bool = 1,
    String = 2,
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