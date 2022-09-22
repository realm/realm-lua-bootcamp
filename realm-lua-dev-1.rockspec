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
         ["realm.object"] = "lib/realm/object.lua",
         ["realm.results"] = "lib/realm/results.lua",
         ["realm.classes"] = "lib/realm/classes.lua"
      }
   }
}

dependencies = {
   "lua >= 5.4"
}

test_dependencies = {
  "busted",
}

test = {
  type = "busted",
}
