---@module "realm.scheduler"

---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---Externally defined classes
---@see Realm.App
---@see Realm.App.User
---@see Realm.Object
---@see Realm.Results
---@see Realm.List
---@see Realm.Set

---@class Realm.Config
---@field path string The path to the realm being opened.
---@field schemaVersion integer The version of the schema for the realm being opened.
---@field schema Realm.Schema.ClassDefinition[] The schema containing all classes and their properties.
---@field scheduler Realm.Scheduler? The scheduler which the realm should be bound to.
---@field sync Realm.Config.Sync? The configuration for opening a synced realm.
---@field _cached boolean? Whether to return a cached Realm instance, default is true.

---@alias Realm.Handle userdata

---@class Realm.ObjectChanges
---@field isDeleted boolean Whether the object has been deleted.
---@field modifiedProperties number[] The property keys of all properties that were modified. -- NOTE: Will change to property names (string[])

---@alias Realm.ObjectChanges.Callback fun(object: Realm.Object, changes: Realm.ObjectChanges) 

---@class Realm.CollectionChanges
---@field deletions number[] The indices of the deleted objects at their previous positions.
---@field insertions number[] The indices of the added objects at their current positions.
---@field modificationsOld number[] The indices of the modified objects at their previous positions.
---@field modificationsNew number[] The indices of the modified objects at their current positions.

---@alias Realm.CollectionChanges.Callback fun(results: Realm.Results, changes: Realm.CollectionChanges)

---@class Realm.Config.Sync
---@field user Realm.App.User The currently logged in user.
---@field partitionValue string The value used for syncing objects with its partition key field set to this value.

---@class Realm.Schema.ClassInformation Schema classes information returned after opening a Realm.
---@field name string The class name.
---@field key integer The class key.
---@field properties table<string, Realm.Schema.PropertyDefinition> The property names containing their definitions.
---@field primaryKey string The property that is the primary key field.

---@class Realm.Schema.ClassDefinition Schema classes definition used to open a Realm.
---@field name string The class name.
---@field primaryKey string? The property that is the primary key field.
---@field properties table<string, Realm.Schema.PropertyDefinition | Realm.Schema.PropertyType> The property names containing their information.

---@class Realm.Schema.PropertyDefinition
---@field type Realm.Schema.PropertyType The data type of the property.
---@field key number? The property key, only defined with properties from ClassInformation.
---@field mapTo string? The new name to map the property name to.
---@field indexed boolean? Whether the property should be indexed.
---@field optional boolean? Whether setting the property can be optional.

---@class Realm.Schema.PropertyInformation
---@field key userdata The property key userdata.
---@field name string The property name.
---@field type PropertyInformation.Type The data type of the property.
---@field objectType string? The object type of the property.
---@field collectionType PropertyInformation.CollectionType? The collection type of the property.

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
