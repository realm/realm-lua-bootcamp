![Realm](https://github.com/realm/realm-dotnet/raw/main/logo.png)

Realm is a mobile database that runs directly on phones, tablets or wearables. This repository holds the source code for the Lua version of [Realm](https://www.mongodb.com/docs/realm/).

# Disclaimer

This SDK does not have any official support but is instead released as a community project. (MongoDB supports several other [Realm SDKs](https://www.mongodb.com/docs/realm/).)

# Features

* **Mobile-first:** Realm is the first database built from the ground up to run directly inside phones, tablets, and wearables.
* **Simple:** Data is directly exposed as objects and queryable by code, removing the need for ORM's riddled with performance & maintenance issues.
* **Modern:** Realm supports relationships.
* **Fast:** Realm is faster than even raw SQLite on common operations while maintaining an extremely rich feature set.
* **Offline-first:** Realm's local database persists data on-disk, so apps work as well offline as they do online.
* **[Device Sync](https://www.mongodb.com/atlas/app-services/device-sync)**: Makes it simple to keep data in sync across users, devices, and your backend in real-time. (This SDK only supports [partition-based sync](https://www.mongodb.com/docs/atlas/app-services/reference/partition-based-sync/).)

# Getting Started

Realm Database uses the concept of a **realm** which is a `.realm` file stored locally on your device. Each realm stores the objects you create that conform to a specific pre-defined schema.

Follow these steps to get started with the Realm Lua SDK.

## Prerequisites

* Install LuaRocks for [Unix](https://github.com/luarocks/luarocks/wiki/Installation-instructions-for-Unix) or [Windows](https://github.com/luarocks/luarocks/wiki/Installation-instructions-for-Windows).
* We recommend [Visual Studio Code](https://code.visualstudio.com/Download) as your IDE using the settings located in [.vscode](.vscode).

## Installation

Start off by cloning this project and navigating to its root folder:

```sh
git clone https://github.com/realm/realm-lua-bootcamp.git
cd realm-lua-bootcamp
```

Install dependencies and packages:

```sh
luarocks install realm-lua-dev-1.rockspec
```

Apply the changes, temporarily, to your shell:

```sh
eval "$(luarocks path --bin)"
```

## Import Realm

Import `Realm` by requiring it at the top of each Lua file that uses Realm.

```Lua
local Realm = require "realm"
```

## Define Your Object Model

Your application's object model defines the data that you can store within Realm Database and synchronize to and from [MongoDB Atlas App Services](https://www.mongodb.com/atlas/app-services) if [Device Sync](https://www.mongodb.com/docs/atlas/app-services/reference/partition-based-sync/) is enabled.

Every Realm object has an *object type*. Objects of the same type share an *object schema* that defines the properties and relationships of those objects.

To define an object schema, create an object that specifies the type's `name`, `properties`, and optionally a `primaryKey` field. The `name` must be unique among all object types in a Realm.

The following example defines schemas for a `Team` object type and a `Task` object type:

```Lua
local TeamSchema = {
    name = "Team",
    primaryKey = "_id",
    properties = {
        _id = "int",
        teamName = "string",
        tasks = "Task[]"
    }
}

local TaskSchema = {
    name = "Task",
    primaryKey = "_id",
    properties = {
        _id = "int",
        description = "string",
        completed = "bool",
        size = "string?",
        assigneeId = "string?"
    }
}
```

The `properties` field specifies all the required and optional properties belonging to the object type being defined. The type of each property also needs to be specified by providing any of the following field data types:

* Primitive type
    * `"string"`
        * A Lua `string`.
    * `"int"`
        * A Lua `number` able to be represented as an integer.
    * `"double"`
        * A Lua `number` containing a non-zero fractional (decimal) part.
    * `"bool"`
        * A Lua `boolean`.
* Collection type
    * `"[]"` (list)
        * A Lua `table` used as an array.
    * `"{}"` (dictionary)
        * A Lua `table` used as a record.
    * `"<>"` (set)
        * A Lua `table` used as a set.
    * When using a collection type, its element type must always be specified (e.g. `"int[]"`).
* The name of a Realm object type.
    * This refers to the string specified in the `name` field of an object schema.
* Nullable type
    * `"?"`
        * Allows a Lua `nil` value.
        * When appended to a primitive or object type (e.g. `"int?"`, `"int?[]"`, or `Task?`), the type becomes nullable, allowing the property value to be set to `nil`.

> ℹ️ This SDK does not support *embedded objects*, i.e. objects that only exist on a single parent object and never as standalone objects. Therefore, all objects must be independent of the other.

## Open a Local (Non-Sync) Realm

To open a realm, pass a configuration object to `Realm.open()` containing a `schema` field with a list of all object schemas belonging to that realm. For our example, we want to store `Team` and `Task` objects in the same realm, so we pass their respective schemas:

```Lua
local realm = Realm.open({
    schema = { TeamSchema, TaskSchema }
})
```

The supported configurations for a Realm are:

* `schema`
    * The list of object schemas belonging to the realm.
* `path`
    * The path (including the name) to where the realm file should be stored.
    * Default: `"default.realm"`
* `schemaVersion`
    * The version of the realm schema.
    * Default: `0`
* `sync`
    * Only for synced realms (see [Open a Synced Realm](#open-a-synced-realm)).

## Create Realm Objects

Once you have opened a realm, you can create objects in it using `realm:create()`. All writes must occur within a write transaction using `realm:write()`:

```Lua
local size = {
    SMALL = "SMALL",
    MEDIUM = "MEDIUM",
    LARGE = "LARGE"
}
local smallTask = {
    _id = math.random(1, 100000),
    description = "Get started with Realm Lua",
    completed = false,
    size = size.SMALL
}
local mediumTask = {
    _id = math.random(1, 100000),
    description = "Build an app using Atlas Device Sync",
    completed = false,
    size = size.MEDIUM
}
local luaTeam = {
    _id = math.random(1, 100000),
    teamName = "Lua",
    tasks = { smallTask, mediumTask }
}

-- Before the write transaction, `smallTask`, `mediumTask`,
-- and `luaTeam` are only regular Lua objects.

realm:write(function ()
    -- `realm:create()` returns the created Realm
    -- object, so we can assign it to our variables.

    smallTask = realm:create("Task", smallTask)
    mediumTask = realm:create("Task", mediumTask)
    luaTeam = realm:create("Team", luaTeam)
end)

-- After the write transaction, the same local
-- variables can now be used as Realm objects.
```

## Query Realm Objects

Querying all objects of a particular type in a realm can be done by passing the object type name to `realm:objects()`:

```Lua
local tasks = realm:objects("Task");

for index, task in ipairs(tasks) do
    print(task.description)
end
```

Once the objects have been queried, they can be filtered using [Realm Query Language](https://www.mongodb.com/docs/realm/realm-query-language/). The following example filters all tasks where `completed` is `false` and `size` is `"SMALL"`:

```Lua
local uncompletedSmallTasks = tasks:filter(
    "completed = $0 AND size = $1",
    false,      -- Replaces $0
    size.SMALL  -- Replaces $1
)

print("Number of uncompleted small tasks: " .. #uncompletedSmallTasks) -- 1
print(uncompletedSmallTasks[1].description) -- "Get started with Realm Lua"
```

> ℹ️ <u>**Live objects:**</u>
> 
> Realm objects and query results are always *live*. This means that they always reflect what is currently stored in the database (realm file). Variables referencing such objects will therefore always be up to date without having to re-query and reassign it.

> ℹ️ <u>**Lazy loading:**</u>
>
> Realm utilizes *lazy loading* for efficiency. This means that calls to `realm:objects()` and `realm:objects():filter()` are not actually executed at that time. Instead, it is executed once an object is accessed, for instance when iterating over the collection or accessing the length or an object of the filtered result.

## Update a Realm Object

As with creating an object, any changes to a Realm object must occur within a write transaction. To modify an object, you simply update its properties:

```Lua
realm:write(function ()
    smallTask.completed = true
end)

print("Number of uncompleted small tasks: " .. #uncompletedSmallTasks) -- 0
```

Lists can be modified via the table's metamethods. The example below creates another `Task` and adds it to the team's list of tasks.

```Lua
local largeTask = {
    _id = math.random(1, 100000),
    description = "Build a great IoT app",
    completed = false,
    size = size.LARGE
}

realm:write(function ()
    realm:create("Task", largeTask)
    table.insert(luaTeam.tasks, largeTask)
end)
```

## Delete a Realm Object

An object can be deleted by passing it to `realm:delete()` within a write transaction:

```Lua
realm:write(function ()
    realm:delete(smallTask)
end)
```

## Get Notified of Changes

**Collection Changes**:

To get notified of changes in a collection (e.g. in the `Team` or `Task` collection) you need to call `<collection>:addListener()` and pass a callback function that will be called whenever an object is deleted, inserted, or modified.

The callback function will be called with two arguments:
1. The collection itself.
2. A table containing the following key value pairs:
    * `deletions`: An array of the indices of the deleted objects at their previous positions.
    * `insertions`: An array of the indices of the added objects at their current positions.
    * `modificationsOld`: An array of the indices of the modified objects at their previous positions.
    * `modificationsNew`: An array of the indices of the modified objects at their current positions.

```Lua
local onTaskCollectionChange = function (collection, changes)
    -- Handle deletions first
    for _, deletedIndex in ipairs(changes.deletions) do
        print("Deleted task at index " .. deletedIndex)
    end

    for _, insertedIndex in ipairs(changes.insertions) do
        print("Added task: " .. collection[insertedIndex].description)
    end

    for _, modifiedIndex in ipairs(changes.modificationsNew) do
        print("Modified task: " .. collection[modifiedIndex].description)
    end
end

-- You currently need to save the return value to a variable
local _ = tasks:addListener(onTaskCollectionChange)
```

> ℹ️ Make sure to always handle potential deletions first.

**Object Changes**:

To get notified of changes to a specific object you need to call `<object>:addListener()` and pass a callback function that will be called whenever the object is deleted or modified.

The callback function will be called with two arguments:
1. The object itself.
2. A table containing the following key value pairs:
    * `isDeleted`: Whether the object has been deleted.
    * `modifiedProperties`: The property names of all properties that were modified.
        > ⚠️ This field currently returns numeric Realm *property keys* (rather than the property names) used only internally, but will be fixed in a later version. The length of this property (`#changes.modifiedProperties`) can still be used to appropriately react to changes.

```Lua
local onTaskObjectChange = function (object, changes)
    if changes.isDeleted then
        print("Deleted a task")
    elseif #changes.modifiedProperties > 0 then
        print("Modified task: " .. object.description)
    end
end

-- You currently need to save the return value to a variable
local _ = largeTask:addListener(onTaskObjectChange)
```

## Add Device Sync (Optional)

If you want to sync Realm data across devices, you can set up an [Atlas App Services App](https://www.mongodb.com/docs/atlas/app-services/manage-apps/create/create-with-ui/) and enable Device Sync.

### Prerequisites

Before you can sync Realm data, you must:

* [Create an App Services App](https://www.mongodb.com/docs/atlas/app-services/manage-apps/create/create-with-ui/)
* Enable one or both of the following authentication providers:
    * [Email/Password Authentication](https://www.mongodb.com/docs/atlas/app-services/authentication/email-password/#std-label-email-password-authentication)
        * This lets users register and log in using an email address.
    * [Anonymous Authentication](https://www.mongodb.com/docs/atlas/app-services/authentication/anonymous/#std-label-anonymous-authentication)
        * This lets users log in without providing credentials. Note that an anonymous user is not intended to persist data. Once a user logs out, the user cannot retrieve any previous user data.
* [Enable Partition-Based Sync](https://www.mongodb.com/docs/atlas/app-services/reference/partition-based-sync/) with **Development Mode** on.
    * Choose a [partition key](https://www.mongodb.com/docs/atlas/app-services/reference/partition-based-sync/#partition-key).
    * > ℹ️ This SDK does not support [Flexible Sync](https://www.mongodb.com/docs/atlas/app-services/sync/configure/enable-sync/).

For our app, we have enabled email/password authentication and chosen `_partition` as the partition key as explained next.

### Define Your Object Model

Objects with the same partition value will be stored in the same realm and allows us to sync only a subset of the total data stored in the cloud (i.e. a partition of it) based on this value.

The partition value in this example will be the name of a team in order to only sync data belonging to a specific team. So, let's add a `_partition` property to the `Team` and `Task` object schemas that we defined in the previous section:

```Lua
local TeamSchema = {
    name = "Team",
    primaryKey = "_id",
    properties = {
        _id = "int",
        -- Add `_partition` (will store the team name):
        _partition = "string",
        teamName = "string",
        tasks = "Task[]"
    }
}

local TaskSchema = {
    name = "Task",
    primaryKey = "_id",
    properties = {
        _id = "int",
        -- Add `_partition` (will store the team name):
        _partition = "string",
        description = "string",
        completed = "bool",
        size = "string?",
        assigneeId = "string?"
    }
}
```

> ℹ️ See [partition strategies](https://www.mongodb.com/docs/atlas/app-services/reference/partition-based-sync/#partition-strategies) for more ways of partitioning your data depending on your use case.

### Initialize the App

To use App Services features, such as authentication and sync, you must first initialize your App using your App ID. You can [find your App ID](https://www.mongodb.com/docs/atlas/app-services/reference/find-your-project-or-app-id/#std-label-find-your-app-id) in the App Services UI.

Import `App` by requiring it at the top of the Lua file that uses it:

```Lua
local App = require "realm.app"
```

Pass a configuration object to `App.new()` containing an `appId` field with the App ID you copied from the App Services UI:

```Lua
local app = App.new({ appId = "<YOUR_APP_ID>" })
```

### Authenticate a User

To authenticate and log in a user they must first register by calling `App:registerEmail()` and passing their email, password, and a callback function that will be called when the registration completes (it will be called with an error message if the registration failed):

(This registration step can be skipped for anonymous logins.)

```Lua
local myEmail = "jane@example.com"
local myPassword = "123456"

app:registerEmail(myEmail, myPassword, function (error)
    -- Called when registration is completed..
end)
```

Once registered, you can call `App:logIn()` with a credentials object and a callback function that will be called when the login completes.

The callback function will be called with one or two arguments:
1. The App User if successfully logged in, otherwise `nil`.
2. An error message if the login failed.

```Lua
-- Email/password credentials
local credentials = App.credentials.emailPassword(myEmail, myPassword)

-- Or anonymous credentials
local credentials = App.credentials.anonymous()

app:logIn(credentials, function (user, error)
    -- Called when login is completed..
end)
```

### Open a Synced Realm

Once you have enabled Device Sync, defined your object model, initialized your App, and authenticated a user, you can open a synced realm.

Similar to opening a non-sync realm, pass a configuration object to `Realm.open()` containing a `schema` field with a list of all object schemas belonging to that realm, and additionally a `sync` field:

```Lua
local realm
local currentUser

app:registerEmail(myEmail, myPassword, function (error)
    -- Called when registration is completed..

    local credentials = App.credentials.emailPassword(myEmail, myPassword)

    app:logIn(credentials, function (user, error)
        -- Called when login is completed..

        currentUser = user
        realm = Realm.open({
            schema = { TeamSchema, TaskSchema },
            sync = {
                user = user,
                -- Add the team's name as the partition value. We will soon create
                -- a new team called "Lua Sync" that we want to sync to this realm.
                -- The value must be the raw Extended JSON string in order to
                -- be compatible with MongoDB documents. (Manually add `"`.)
                partitionValue = "\"Lua Sync\""
            }
        })
    end)
end)

-- The current App User can also be retrieved once logged in using:
local currentUser = app:currentUser()
```

### Create, Read (Query), Update, and Delete Realm Objects

The syntax to create, read/query, update, and delete, as well as getting notified of changes, on a synced realm is identical to the syntax for local (non-synced) realms explained earlier. While you work with local data, a background thread efficiently integrates, uploads, and downloads changesets.

Although, since we added a required `_partition` field to our object schemas, new objects that we create must also contain the partition value. Objects without such a field, or those with a partition value that does not equal the team name specified when opening the realm, will not be synced to the device.

```Lua
local syncedTask = {
    _id = math.random(1, 100000),
    -- Provide the partition value
    _partition = "Lua Sync",
    description = "Modify data in MongoDB Atlas and watch it sync",
    completed = false,
    size = size.SMALL,
    -- Optionally set the current user as the assignee
    assigneeId = currentUser.identity
}
local syncedTeam = {
    _id = math.random(1, 100000),
    -- Provide the partition value
    _partition = "Lua Sync",
    teamName = "Lua Sync",
    tasks = { syncedTask }
}

realm:write(function ()
    syncedTask = realm:create("Task", syncedTask)
    syncedTeam = realm:create("Team", syncedTeam)
end)
```

# Examples

Some minimal examples of Realm use can be found in [main.lua](main.lua). The examples are similar to the code demonstrated in this README but also includes error handling.

# Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details!

# Code of Conduct

This project adheres to the [MongoDB Code of Conduct](https://www.mongodb.com/community-code-of-conduct). By participating, you are expected to uphold this code. Please report
unacceptable behavior to [community-conduct@mongodb.com](mailto:community-conduct@mongodb.com).

# License

Realm Lua and [Realm Core](https://github.com/realm/realm-core) are published under the Apache License 2.0.

# Feedback

**_If you use Realm and are happy with it, all we ask is that you please consider sending out a tweet mentioning [@realm](https://twitter.com/realm) to share your thoughts!_**

**_And if you don't like it, please let us know what you would like improved, so we can fix it!_**

<img style="width: 0px; height: 0px;" src="https://3eaz4mshcd.execute-api.us-east-1.amazonaws.com/prod?s=https://github.com/realm/realm-lua-bootcamp#README.md">
