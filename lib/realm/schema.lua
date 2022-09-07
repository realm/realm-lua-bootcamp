---@meta

---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---@class Realm.Schema.PropertyDefinition
---@field type Realm.Schema.PropertyType
---@field mapTo? string
---@field indexed? boolean
---@field optional? boolean

---@class Realm.Schema.ClassDefinition
---@field name string
---@field primaryKey? string
---@field properties table<string, Realm.Schema.PropertyDefinition | Realm.Schema.PropertyType>
