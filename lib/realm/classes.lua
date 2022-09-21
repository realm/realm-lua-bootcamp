---@meta

---@alias Realm.Schema.PropertyType string | "bool" | "int" | "float" | "string"

---Externally defined classes
---@see Realm.Object
---@see Realm.Results

---@class Realm.Config
---@field path string
---@field schemaVersion integer
---@field schema Realm.Schema.ClassDefinition[]

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
