-- Hello Tasia - example Lua plugin
local version = tasia.viewer.getVersion()
tasia.viewer.notify("Hello from Tasia! Viewer version: " .. version.version)

-- Subscribe to chat
tasia.events.subscribe("chat.received", function(data)
    if data.message:lower():find("hello") then
        tasia.chat.send("Hi there! Plugin says hello back.")
    end
end)

tasia.events.subscribe("region.changed", function(data)
    tasia.viewer.notify("Arrived at: " .. data.region_name)
end)
