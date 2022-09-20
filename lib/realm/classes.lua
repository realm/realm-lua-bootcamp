---@meta

---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---@class Realm.Config
---@field path string
---@field schemaVersion integer
---@field schema Realm.Schema.ClassDefinition[]

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
