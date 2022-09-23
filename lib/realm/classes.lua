---@meta

---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---@class Realm.Config
---@field path string
---@field schemaVersion integer
---@field schema Realm.Schema.ClassDefinition[]
---@field sync Realm.Config.Sync?

---@class Realm.Config.Sync
---@field user RealmUser The currently logged in user.
---@field partitionValue string The value used for syncing objects with its partition key field set to this value.

---@class Realm.Schema.ClassInformation Schema classes information returned after opening a Realm
---@field key integer Class key
---@field properties table<string, Realm.Schema.PropertyDefinition>

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
