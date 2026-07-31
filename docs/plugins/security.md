# Plugin Security

## Design Principles

1. **Untrusted by default**: All plugins are treated as untrusted code.
2. **Least privilege**: Plugins must request specific permissions.
3. **Isolation**: Lua/JS runtimes are embedded with safety restrictions. Python runs in a separate process.
4. **No raw pointer access**: Plugins never access viewer C++ pointers or internal objects.
5. **Auditable**: All API calls go through LLPluginService with permission checks.

## Runtime Security

### Lua
- Separate Lua state per plugin
- No `os.execute()`, `io.*`, `loadlib()`, `dofile()` in default sandbox
- Restricted filesystem access
- Memory limits via Lua allocator

### JavaScript (QuickJS)
- Separate JS context per plugin
- No `eval()` on arbitrary strings without permission
- No `require()` or native module loading
- Restricted timer resolution

### Python
- Runs in a separate process (`tasia-plugin-host-python.py`)
- IPC via named pipes (Windows) or Unix sockets (Linux)
- A crash in a Python plugin does not crash the viewer
- Subprocess calls are disabled by policy
- Restricted filesystem access

## Protocol Security

- Maximum message size: 256 KB
- Maximum method name length: 128 characters
- Parameter depth limit: 16 levels
- JSON parsing rejects malformed input
- Path traversal detection in all file operations
- Rate limiting: 60 events/second per plugin
- Callback time limit: 500ms per handler

## Safe Mode

- `--safe-mode`: Disables all user plugins
- `--disable-plugins`: Disables plugins without safe mode
- Crash tracking: Crashed plugin IDs are saved to a marker file
- On next startup, safe mode activates automatically
- User can disable or remove the failing plugin via the Plugin Manager

## What's NOT Implemented (v1)

- Native .dll/.so/.dylib plugins (design only, loading disabled)
- Plugin package signing and verification
- Encrypted plugin storage
- Network-level sandboxing (e.g., firewall per plugin)
- Memory limits for Python host process
