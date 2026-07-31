# Plugin Development Guide

## Getting Started

### 1. Create a plugin directory

```bash
mkdir -p plugins/my-plugin
cd plugins/my-plugin
```

### 2. Create manifest.json

```json
{
    "id": "com.example.myplugin",
    "name": "My Plugin",
    "version": "1.0.0",
    "author": "You",
    "description": "My first Tasia plugin",
    "entrypoint": "main.lua",
    "runtime": "lua",
    "api_version": 1,
    "permissions": ["viewer.notify"]
}
```

### 3. Create your script

```lua
-- main.lua
tasia.viewer.notify("Hello from My Plugin!")

tasia.events.subscribe("chat.received", function(data)
    if data.message:lower():find("ping") then
        tasia.chat.send("pong!")
    end
end)
```

### 4. Enable the plugin

1. Launch Tasia Viewer
2. Go to Me → Tasia Plugins
3. Find your plugin in the list
4. Click "Enable"
5. Grant the requested permissions

## Debugging

### Plugin Logs

Each plugin has its own log file:
- Linux: `~/.config/tasia/logs/plugins/<plugin-id>.log`
- Windows: `%APPDATA%/Tasia/logs/plugins/<plugin-id>.log`

### Developer Console

The Plugin Manager includes a "View Logs" button that opens the log directory.

### Common Issues

**"Runtime not available"**:
The required runtime (Lua/JS/Python) was not compiled into this viewer build.
Enable it via CMake options and rebuild.

**"Permission denied"**:
The plugin is missing required permissions. Check via the Permissions button.

**"Manifest validation failed"**:
The manifest.json has errors. Check the log for details.

## Scripting Runtimes

### Lua
- Lua 5.4 syntax
- Sandboxed: no os.execute(), io.*, dofile(), loadlib()
- Separate state per plugin
- Example: `examples/plugins/hello-lua/`

### JavaScript
- QuickJS engine (not Node.js, not Electron)
- ES2020 compatible
- Separate context per plugin
- Example: `examples/plugins/hello-js/`

### Python
- Runs in separate process via IPC
- CPython interpreter
- No subprocess, restricted filesystem
- Example: `examples/plugins/hello-python/`

## Best Practices

1. **Request minimum permissions**: Only ask for what you need
2. **Handle errors gracefully**: Wrap API calls in error handlers
3. **Clean up subscriptions**: Unsubscribe from events when done
4. **Be responsive**: Keep event handlers fast (< 500ms)
5. **Test in safe mode**: Verify your plugin works with safe mode on
6. **Use versioned IDs**: Follow reverse-domain convention for plugin IDs
