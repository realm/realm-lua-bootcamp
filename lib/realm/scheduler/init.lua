
local scheduler = {}

--- @class Realm.Scheduler : userdata

---A default factory for Realm.Scheduler. Must be set if not explicitly opening realms with a predefined scheduler
---@return Realm.Scheduler
function scheduler.defaultFactory()
    error("No scheduler has been set.")
end

return scheduler