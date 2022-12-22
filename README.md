![Realm](https://github.com/realm/realm-dotnet/raw/main/logo.png)

Realm is a mobile database that runs directly on phones, tablets or wearables. This repository holds the source code for the Lua version of Realm. 

# Disclaimer

This project does not have any official support but is instead released as a community project.

# Features

* **Mobile-first:** Realm is the first database built from the ground up to run directly inside phones, tablets, and wearables.
* **Simple:** Data is directly [exposed as objects](https://docs.mongodb.com/realm/dotnet/objects/) and [queryable by code](https://docs.mongodb.com/realm/dotnet/query-engine/), removing the need for ORM's riddled with performance & maintenance issues. Plus, we've worked hard to [keep our API down to just a few common classes](https://docs.mongodb.com/realm-sdks/dotnet/latest/): most of our users pick it up intuitively, getting simple apps up & running in minutes.
* **Modern:** Realm supports relationships, generics, vectorization and modern C# idioms.
* **Fast:** Realm is faster than even raw SQLite on common operations while maintaining an extremely rich feature set.
* **[Device Sync](https://www.mongodb.com/atlas/app-services/device-sync)**: Makes it simple to keep data in sync across users, devices, and your backend in real-time. [Get started](http://mongodb.com/realm/register?utm_medium=github_atlas_CTA&utm_source=realm_dotnet_github) for free with a template application that includes a cloud backend and Sync.

# Getting Started
Start off by cloning this project into your codebase. In this projects root fulder run `luarocks install realm-lua-dev-1.rockspec`. Once it's complete, run `eval $(luarocks path)` to apply the changes to your shell.

# Usage

## Defining a schema

Start of by defining your schema.

```Lua
schema = {
    {
        name = "Person",
        primaryKey = "_id",
        properties = {
            _id = "int",
            name = "string"
            age = "int"
            favoriteDog = "Dog?",
            dogs = "Dog[]"
        }
    },
    {
        name = "Dog",
        primaryKey = "_id",
        properties = {
            _id = "int",
            name = "string"
            age = "int"
        }
    }
}
```

for `properties` we currently support ints, doubles, strings, booleans, links to other objects and lists of objects. Note that object links must be appended with "?" which means that they are nullable.

## Open Database

Once you have a schema you can open the database.

```Lua
local Realm = require "realm"
local realm = Realm.open(schema)
```

## Write

Persist some data by instantiating the model object and copying it into the open Realm instance during a write transaction.

```Lua
local dog = {
    _id = math.random(1, 100000),
    name = "Max",
    age = 5
}

local person = {
    _id = math.random(1, 100000),
    name = "John",
    age = 42,
    favoriteDog = dog
}
    
local persistedDog 
local persistedPerson 
realm:write(function()
    persistedDog = realm:create("Dog", dog)
    persistedPerson = realm:create("Person", person)
end)
```

## Read

Accessing properties of an object is done with dot-notation.

```Lua
print(dog.age) -- outputs 5
```


## Update

Updating data is also done within a write transaction.

```Lua
realm:write(function()
    persistedDog.age = "6"
    persistedPerson.name = "Doe"
end)
```

## Delete

Finally you can delete objects from the database within a transaction.

```Lua
realm:write(function()
    realm:delete(persistedDog)
end)
```

## Collections & filter

To fetch all objects from a collection you can use the `realm:objects()`method and provide the object name.

```Lua
local allPeople = realm:objects("Person")
```

You can then perform filters on that collection using the Realm Query Language (RQL).

```Lua
local filteredPeople = people:filter(
    “name = $0 AND age = $1”,
    “Doe”,
    42
)
```

You can also interact with a list by using a table's metamethods.

```Lua
local otherDog = {
    _id = math.random(1, 100000),
    name = "Milo",
    age = 4
}
realm:write(function()
    table.insert(persistedPerson.dogs, otherDog) 
end)
print(#persistedPerson.dogs) -- outputs 1
print(persistedPerson[1].name) -- outputs Milo
```

## Notifications

To subscribe on events on a collection you need to invoke its `addListener` method and provide a callback function.

```Lua
local persons = realm:objects(“Person”) 
persons:addListener(function (collection, changes)
    for _, index in ipairs(changes.deletions) do
        print(“Deleted store.”)
        print(“Index: ” .. index)
    end

    for _, index in ipairs(changes.insertions) do
        print(“Added store.”)
        print(“Id: ” .. collection[index]._id)
    end

    for _, index in ipairs(changes.modificationsNew) do
        print(“Modified store.”)
        print(“Id: ” .. collection[index]._id)
    end
end)
```

## Using a synced Realm
Using a synced Realm requires you to initially register a user.

```Lua
local App = require “realm.app”
local APP_ID = “MY_APP_ID”
local app = App.new({ appId = APP_ID })
local realm
app:registerEmail(email, password, function(err)
    local credentials = App
        .credentials
        .emailPassword(email, password)

    app:logIn(credentials, function(user, err)
        realm = openRealm(user)
    end)
end)

local currentUser = app:currentUser()
```

Opening a Realm now requires an additional `sync` field in the schema.

```Lua
return Realm.open({
    schema = {
        {
            name = “Store”,
            primaryKey = “_id”,
            properties = {
                _id = “int”,
                city = “string”
            }
        }
    }
    sync = {
        user = currentUser,
        partitionValue = “\”Chicago\””
    }
})
```

The only sync mode supported is [partition-based sync](https://www.mongodb.com/docs/atlas/app-services/reference/partition-based-sync/). For additional information on how to setup sync see [this](https://www.mongodb.com/docs/atlas/app-services/sync/get-started/).


# Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details!

# Code of Conduct

This project adheres to the [MongoDB Code of Conduct](https://www.mongodb.com/community-code-of-conduct).
By participating, you are expected to uphold this code. Please report
unacceptable behavior to [community-conduct@mongodb.com](mailto:community-conduct@mongodb.com).

# License

Realm Lua and [Realm Core](https://github.com/realm/realm-core) are published under the Apache License 2.0.

# Feedback

**_If you use Realm and are happy with it, all we ask is that you please consider sending out a tweet mentioning [@realm](https://twitter.com/realm) to share your thoughts!_**

**_And if you don't like it, please let us know what you would like improved, so we can fix it!_**

<img style="width: 0px; height: 0px;" src="https://3eaz4mshcd.execute-api.us-east-1.amazonaws.com/prod?s=https://github.com/realm/realm-dotnet#README.md">
