local uv = require "luv"
local scheduler = require "realm.scheduler"
local native = require "realm.scheduler.libuv.native"

scheduler.defaultFactory = function()
    local async, scheduler, userdata

    async = uv.new_async(function()
        if (userdata:should_close()) then
            async:close(function()
                userdata:close()
            end)
        else
            userdata:do_work();
        end
    end)

    userdata, scheduler = native.create_scheduler(async)
    return scheduler
end
