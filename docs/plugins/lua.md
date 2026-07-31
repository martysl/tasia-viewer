# Lua Plugin Runtime

The Lua runtime uses **SLua** — Second Life's official fork of **Luau** (Roblox's Lua).
SLua is a fast, safe, gradually typed embeddable scripting language derived from Lua 5.1.

## Source

- **SLua repo:** https://github.com/secondlife/slua
- **Luau upstream:** https://github.com/Roblox/luau
- **Wiki:** https://wiki.secondlife.com/wiki/Lua_Alpha

## Building with Lua Support

```bash
# Clone SLua alongside the viewer
git clone https://github.com/secondlife/slua.git

# Build SLua
cd slua
mkdir cmake && cd cmake
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build . --target Luau.VM --config RelWithDebInfo
cmake --build . --target Luau.Compiler --config RelWithDebInfo
cd ../../tasia-new-viewer

# Build viewer with Lua enabled
mkdir build && cd build
cmake .. -DTASIA_ENABLE_LUA=ON -DSLUA_DIR=/path/to/slua
make -j$(nproc)
```

## Sandboxing

Plugins run in a sandboxed Lua environment:

- **`luaL_sandbox()`** — sandboxes the global state, protects builtins from monkey-patching
- **`luaL_sandboxthread()`** — sandboxes individual script threads
- **Custom allocator** — 64 MB memory limit per plugin
- **Removed globals**: `dofile`, `loadfile`, `require`
- **Safe libraries only**: base, table, string, math, utf8, coroutine
- **No IO**, no OS library, no debug library

## API Bridge

Lua functions are registered as C closures in the `tasia` table:

```lua
tasia.viewer.notify("Hello!")
tasia.chat.send("Hi there", 0)
```

Event handlers are stored in `_tasia_events` table and called by the host:

```lua
-- In plugin script:
_tasia_events = _tasia_events or {}
_tasia_events["chat.received"] = function(data)
    tasia.viewer.notify("Chat: " .. data.message)
end
```

## Dependencies

- C++17 or later
- Luau.VM library
- Luau.Compiler library
- No external Lua installation required

## Limitations (current version)

- Only safe subset of Lua libraries loaded
- No os.execute(), io.*, or debug.*
- No JIT (Luau's JIT works but disabled in sandboxed mode)
- Memory limited to 64 MB per plugin
- No coroutine persistence across viewer restart
