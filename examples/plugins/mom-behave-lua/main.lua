-- Mom Behave - example plugin using Mom API
-- When Mom enters the sim, behave. When she leaves, mischief mode.

local behaving = false

local function update_behavior()
    if tasia.mom.isOnSim() then
        behaving = true
        tasia.viewer.notify("Mom is here. Innocent mode activated.")
    else
        behaving = false
        tasia.viewer.notify("Mom left the sim. Mischief mode restored.")
    end
end

-- Subscribe to Mom events
tasia.events.subscribe("mom.entered_sim", update_behavior)
tasia.events.subscribe("mom.left_sim", update_behavior)

-- Check initial state
update_behavior()

-- Example: periodically check distance
tasia.events.subscribe("mom.distance_changed", function(data)
    if data.distance < 20 then
        tasia.viewer.notify("Mom is very close! Distance: " .. data.distance .. "m")
    end
end)

-- Example behavior functions (plugin-local only)
function tasia.behave()
    -- Plugin-local: close risky floaters, stop mischief
    -- These are example callbacks, not viewer security bypasses
    behaving = true
    tasia.viewer.notify("Behaving now.")
end

function tasia.mischief()
    -- Plugin-local: resume fun activities
    -- Does NOT modify viewer security or permissions
    behaving = false
    tasia.viewer.notify("Mischief mode engaged.")
end
