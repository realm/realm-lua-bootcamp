rockspec_format = "3.0"

package = "realm-lua"
version = "dev-1"
source = {
   url = "git+ssh://git@github.com/realm/realm-lua-bootcamp.git"
}
description = {
   homepage = "https://www.realm.io",
   license = "Apache-2.0"
}

build = {
   type = "cmake",
   variables = {
      LUAROCKS="ON",
      CMAKE_C_FLAGS="$(CFLAGS)",
      CMAKE_CXX_FLAGS="$(CFLAGS)",
      CMAKE_MODULE_LINKER_FLAGS="$(LIBFLAG)",
      LUA_INCLUDE_DIR="$(LUA_INCDIR)",
      CMAKE_INSTALL_PREFIX="$(PREFIX)",
   },
   platforms = {
      windows = {
         variables = {
            LUA_LIBRARIES="$(LUA_LIBDIR)/$(LUALIB)",
         }
      }
   },
   install = {
      lua = {
         ["realm"] = "lib/realm/init.lua",
         ["realm.app"] = "lib/realm/app.lua",
         ["realm.app.user"] = "lib/realm/app/user.lua",
         ["realm.app.credentials"] = "lib/realm/app/credentials.lua",
         ["realm.list"] = "lib/realm/list.lua",
         ["realm.set"] = "lib/realm/set.lua",
         ["realm.object"] = "lib/realm/object.lua",
         ["realm.results"] = "lib/realm/results.lua",
         ["realm.classes"] = "lib/realm/classes.lua",
         ["realm.scheduler"] = "lib/realm/scheduler/init.lua",
         ["realm.scheduler.libuv"] = "lib/realm/scheduler/libuv.lua"
      }
   }
}

dependencies = {
   "lua >= 5.4",
   "luv"
}

test_dependencies = {
  "busted",
  "inspect"
}

test = {
  type = "busted",
}
