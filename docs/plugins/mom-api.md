# Mom API

The Mom API allows plugins to detect and react to a configured trusted avatar ("Mom").

## Configuration

Set in viewer Preferences → Tasia → Mom:

| Setting | Description |
|---------|-------------|
| Mom Avatar UUID | The UUID of the Mom avatar to track |
| Mom Avatar Name | Display name (for reference) |
| Enable Mom API | Must be enabled for API to work |

## API Methods

Requires `mom.read` permission.

### tasia.mom.isOnline()

Returns whether the Mom avatar is online.

```lua
if tasia.mom.isOnline() then
    tasia.viewer.notify("Mom is online!")
end
```

### tasia.mom.isOnSim()

Returns whether the Mom avatar is in the same sim/region.

```lua
if tasia.mom.isOnSim() then
    -- behave!
end
```

### tasia.mom.distance()

Returns the distance to Mom in meters, or -1 if not available.

```lua
local dist = tasia.mom.distance()
if dist >= 0 and dist < 20 then
    tasia.viewer.notify("Mom is very close!")
end
```

### tasia.mom.uuid()

Returns the configured Mom avatar UUID.

### tasia.mom.name()

Returns the configured Mom avatar display name.

## Events

Requires `mom.events` permission.

| Event | Description |
|-------|-------------|
| `mom.online` | Mom came online |
| `mom.offline` | Mom went offline |
| `mom.entered_sim` | Mom entered the current sim |
| `mom.left_sim` | Mom left the current sim |
| `mom.distance_changed` | Distance to Mom changed; data includes `distance` field |

## Example Plugin

See `examples/plugins/mom-behave-lua/` for a complete example that:
1. Subscribes to Mom presence events
2. Shows notifications when Mom enters/leaves the sim
3. Tracks distance changes
4. Demonstrates `tasia.behave()` and `tasia.mischief()` as local state callbacks

The `tasia.behave()` and `tasia.mischief()` functions are plugin-local example state setters. They do NOT alter viewer security settings, permissions, or bypass any restrictions. They serve as demonstration callbacks for plugin behavior patterns.
