![Realm](https://github.com/realm/realm-dotnet/raw/main/logo.png)

Realm is a mobile database that runs directly on phones, tablets or wearables. This repository holds the source code for the Lua version of Realm. 

# Features

* **Mobile-first:** Realm is the first database built from the ground up to run directly inside phones, tablets, and wearables.
* **Simple:** Data is directly [exposed as objects](https://docs.mongodb.com/realm/dotnet/objects/) and [queryable by code](https://docs.mongodb.com/realm/dotnet/query-engine/), removing the need for ORM's riddled with performance & maintenance issues. Plus, we've worked hard to [keep our API down to just a few common classes](https://docs.mongodb.com/realm-sdks/dotnet/latest/): most of our users pick it up intuitively, getting simple apps up & running in minutes.
* **Modern:** Realm supports relationships, generics, vectorization and modern C# idioms.
* **Fast:** Realm is faster than even raw SQLite on common operations while maintaining an extremely rich feature set.
* **[Device Sync](https://www.mongodb.com/atlas/app-services/device-sync)**: Makes it simple to keep data in sync across users, devices, and your backend in real-time. [Get started](http://mongodb.com/realm/register?utm_medium=github_atlas_CTA&utm_source=realm_dotnet_github) for free with a template application that includes a cloud backend and Sync.

# Getting Started

# Getting Help

# Building Realm




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

for `properties` we currently support numbers, strings, booleans, links to other objects and lists of objects. Note that object links must be appended with "?" which means that they are nullable.

## Open Database

Once you have a schema you can open the database.

```Lua
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
    print(dog.name) -- outputs 5
```


## Update

Updating data is also performed within a write transaction.

```Lua
    realm:write(function()
        persistedDog.name = "6"
        persistedPerson.name = "doe"
    end)
```

## Delete

Finally you can delete objects from the database within a transaction.

```Lua
    realm:write(function()
        realm:delete(persistedPerson)
    end)
```

## Lists

You can add elements to a list by using table method "insert"

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
    print(#persistedPerson[1].name) -- outputs Milo
```


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
