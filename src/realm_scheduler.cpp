#include <thread>
#include <realm.h>

#include "realm_scheduler.hpp"
#include "realm_util.hpp"

#include <realm/object-store/util/scheduler.hpp>
#include <realm/object-store/c_api/types.hpp>

namespace luv {

static const char* UserdataMeta = "realm_scheduler_luv_userdata";

typedef struct uv_async_s uv_async_t;
typedef int(*uv_async_send_t)(uv_async_t*);

class Scheduler : public realm::util::Scheduler {
public:
    struct Userdata {
        realm::util::InvocationQueue queue;

        volatile bool close_requested;
        bool closed = false;

        ~Userdata() {
            closed = true;
        }
    };

    Scheduler(uv_async_t* async, uv_async_send_t send, Userdata* userdata)
    : m_async(async)
    , m_send(send)
    , m_userdata(userdata)
    { }

    ~Scheduler() {
        m_userdata->close_requested = true;
        m_send(m_async);
    }

    virtual void invoke(realm::util::UniqueFunction<void()>&& fn) final {
        m_userdata->queue.push(std::move(fn));
        m_send(m_async);
    };

    virtual bool is_on_thread() const noexcept final {
        return m_thread == std::this_thread::get_id();
    }

    virtual bool is_same_as(const realm::util::Scheduler* other) const noexcept final {
        if (auto other_scheduler = dynamic_cast<const Scheduler*>(other)) {
            return m_async == other_scheduler->m_async;
        }
        return false;
    }

    virtual bool can_invoke() const noexcept final { return true; }
private:
    std::thread::id m_thread = std::this_thread::get_id();

    uv_async_t* m_async;
    uv_async_send_t m_send;

    Userdata* m_userdata;
};

#if defined(_WIN32)
#include <libloaderapi.h>
static void* find_function(void* lib, const char* name) {
    return GetProcAddress(static_cast<HMODULE>(lib), name);
}
#else
#include <dlfcn.h>
static void* find_function(void* lib, const char* name) {
    return dlsym(lib, name);
}
#endif

static int create_scheduler(lua_State* L) {
    uv_async_t* async = *static_cast<uv_async_t**>(luaL_checkudata(L, 1, "uv_async"));
    //int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_getfield(L, LUA_REGISTRYINDEX, "_CLIBS");
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        if (lua_type(L, -2) == LUA_TSTRING && ends_with(lua_tostringview(L, -2), "luv.so")) {
            void* lib = lua_touserdata(L, -1);
            // Pop the key off the stack, we'll be returning early.
            lua_pop(L, 1);

            auto uv_async_send = reinterpret_cast<int(*)(uv_async_t*)>(find_function(lib, "uv_async_send"));
            if (!uv_async_send) {
                return _inform_error(L, "Could not find the libuv symbols in the luv module.");
            }

            auto userdata = static_cast<Scheduler::Userdata*>(lua_newuserdata(L, sizeof(Scheduler::Userdata)));
            new (userdata) Scheduler::Userdata;
            luaL_setmetatable(L, UserdataMeta);

            auto scheduler = static_cast<realm_scheduler_t**>(lua_newuserdata(L, sizeof(realm_scheduler_t*)));
            *scheduler = new realm_scheduler_t(std::make_shared<Scheduler>(async, uv_async_send, userdata));
            luaL_setmetatable(L, RealmHandle);
            
            return 2;
        }
        // Pop the last key off the stack.
        lua_pop(L, 1);
    }

    return _inform_error(L, "Could not find the luv native module.");
}

static int should_close(lua_State* L) {
    auto userdata = static_cast<Scheduler::Userdata*>(luaL_checkudata(L, 1, UserdataMeta));
    lua_pushboolean(L, userdata->close_requested);

    return 1;
}

static int close(lua_State* L) {
    auto userdata = static_cast<Scheduler::Userdata*>(luaL_checkudata(L, 1, UserdataMeta));
    if (!userdata->closed) {
        userdata->~Userdata();
    }

    return 0;
}

static int do_work(lua_State* L) {
    auto userdata = static_cast<Scheduler::Userdata*>(luaL_checkudata(L, 1, UserdataMeta));
    userdata->queue.invoke_all();

    return 0;
}

extern "C" int luaopen_realm_scheduler_libuv_native(lua_State* L) {
    luaL_Reg userdata_meta[] = {
        {"__gc", close},
        {NULL, NULL}
    };
    luaL_newmetatable(L, UserdataMeta);
    luaL_setfuncs(L, userdata_meta, 0);

    luaL_Reg userdata_funcs[] {
        {"should_close", should_close},
        {"close", close},
        {"do_work", do_work},
        {NULL, NULL}
    };
    luaL_newlib(L, userdata_funcs);
    lua_setfield(L, -2, "__index");
    
    lua_pop(L, 1); // pop the userdata metatable

    luaL_Reg funcs[] = {
        {"create_scheduler", create_scheduler},
        {NULL, NULL}
    };
    luaL_newlib(L, funcs);

    return 1;
}

}
