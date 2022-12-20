![Realm](https://github.com/realm/realm-dotnet/raw/main/logo.png)

Realm is a mobile database that runs directly on phones, tablets or wearables. This repository holds the source code for the Lua version of Realm. 

## Features

* **Mobile-first:** Realm is the first database built from the ground up to run directly inside phones, tablets, and wearables.
* **Simple:** Data is directly [exposed as objects](https://docs.mongodb.com/realm/dotnet/objects/) and [queryable by code](https://docs.mongodb.com/realm/dotnet/query-engine/), removing the need for ORM's riddled with performance & maintenance issues. Plus, we've worked hard to [keep our API down to just a few common classes](https://docs.mongodb.com/realm-sdks/dotnet/latest/): most of our users pick it up intuitively, getting simple apps up & running in minutes.
* **Modern:** Realm supports relationships, generics, vectorization and modern C# idioms.
* **Fast:** Realm is faster than even raw SQLite on common operations while maintaining an extremely rich feature set.
* **[Device Sync](https://www.mongodb.com/atlas/app-services/device-sync)**: Makes it simple to keep data in sync across users, devices, and your backend in real-time. [Get started](http://mongodb.com/realm/register?utm_medium=github_atlas_CTA&utm_source=realm_dotnet_github) for free with a template application that includes a cloud backend and Sync.

## Getting Started

Prerequitsites:
* LuaRocks

Install the Realm package by running the command
```
luarocks install realm
```
To then use Realm add the following line to your file:
```
require("realm")
```


## Getting Help

## Building Realm

## Examples

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details!

## Code of Conduct

This project adheres to the [MongoDB Code of Conduct](https://www.mongodb.com/community-code-of-conduct).
By participating, you are expected to uphold this code. Please report
unacceptable behavior to [community-conduct@mongodb.com](mailto:community-conduct@mongodb.com).

## License

Realm Lua and [Realm Core](https://github.com/realm/realm-core) are published under the Apache License 2.0.

## Feedback

**_If you use Realm and are happy with it, all we ask is that you please consider sending out a tweet mentioning [@realm](https://twitter.com/realm) to share your thoughts!_**

**_And if you don't like it, please let us know what you would like improved, so we can fix it!_**

<img style="width: 0px; height: 0px;" src="https://3eaz4mshcd.execute-api.us-east-1.amazonaws.com/prod?s=https://github.com/realm/realm-dotnet#README.md">
