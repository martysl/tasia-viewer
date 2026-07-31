# Tasia Viewer Plugin System

## Overview

The Tasia Viewer Plugin System allows third-party scripts to extend the viewer with custom functionality using Lua, JavaScript, or Python. All runtimes share the same API and communicate through a versioned JSON-RPC 2.0 protocol.

## Quick Start

1. Create a plugin directory under `plugins/` (next to the viewer binary)
2. Add a `manifest.json` and your script file
3. Open the Plugin Manager (Me → Tasia Plugins)
4. Enable your plugin

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Tasia Viewer                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────┐ │
│  │ Plugin   │  │ Plugin   │  │ Plugin   │  │ Plugin     │ │
│  │ Manager  │  │ Service  │  │ EventBus │  │ Permissions│ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └─────┬──────┘ │
│       │              │             │               │        │
│  ┌────┴──────────────┴─────────────┴───────────────┴──────┐ │
│  │              Runtime Hosts                              │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐     │ │
│  │  │ Lua Host │  │  JS Host │  │ Python IPC Host  │     │ │
│  │  └────┬─────┘  └────┬─────┘  └────────┬─────────┘     │ │
│  └───────┴─────────────┴──────────────────┴──────────────┘ │
│           │              │                   │              │
│  ┌────────┴──────────────┴───────────────────┴──────────┐  │
│  │              Plugin Scripts                           │  │
│  │  main.lua    main.js    main.py                       │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Key Components

- **LLPluginManager**: Discovers, loads, unloads plugins
- **LLPluginService**: Dispatches API calls to viewer subsystems
- **LLPluginEventBus**: Event subscription and dispatch
- **LLPluginPermissions**: Permission enforcement
- **LLPluginManifest**: Manifest parsing and validation
- **LLPluginRuntime**: Abstract base for runtime hosts
- **LLPluginProtocol**: JSON-RPC 2.0 message protocol
- **LLPluginStorage**: Per-plugin key-value storage
- **LLFloaterPluginManager**: Plugin manager UI

## Plugin Locations

Plugins are discovered from:
- `./plugins/` (next to viewer executable)
- User config directory: `~/.config/tasia/plugins/` (Linux) or `%APPDATA%/Tasia/plugins/` (Windows)

## Command-Line Options

- `--disable-plugins`: Disable all plugins
- `--safe-mode`: Safe mode - disables plugins, shows crash info

## Build Options

- `TASIA_ENABLE_PLUGINS=ON` (default: ON)
- `TASIA_ENABLE_LUA=ON` (default: OFF, requires Lua 5.4)
- `TASIA_ENABLE_JAVASCRIPT=ON` (default: OFF, requires QuickJS)
- `TASIA_ENABLE_PYTHON=ON` (default: OFF, requires CPython)
