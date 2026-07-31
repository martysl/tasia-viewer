#!/usr/bin/env python3
"""
Tasia Viewer - Python Plugin Host
Communicates with the viewer via IPC (named pipes / Unix sockets).
Receives JSON-RPC 2.0 messages and dispatches to Python plugins.

Usage:
    tasia-plugin-host-python.py <pipe_path> <plugin_dir>
"""

import sys
import json
import os
import traceback

# Plugin API namespace
class TasiaAPI:
    """Bridge between Python plugins and the viewer via IPC."""
    
    def __init__(self, send_fn):
        self._send = send_fn
        self._event_handlers = {}
    
    def _request(self, method, params=None):
        """Send an API request to the viewer and wait for response."""
        import uuid
        msg_id = abs(hash(uuid.uuid4()))
        msg = {
            "jsonrpc": "2.0",
            "id": msg_id,
            "method": method,
            "params": params or {}
        }
        self._send(json.dumps(msg))
        # In real implementation, we'd wait for response via IPC
        return {"success": True}
    
    class viewer:
        @staticmethod
        def get_version(api): return api._request("viewer.getVersion")
        @staticmethod
        def get_grid(api): return api._request("viewer.getGrid")
        @staticmethod
        def notify(api, message): return api._request("viewer.notify", {"message": message})
        @staticmethod
        def open_floater(api, name): return api._request("viewer.openFloater", {"name": name})
    
    class chat:
        @staticmethod
        def send(api, message, channel=0):
            return api._request("chat.send", {"message": message, "channel": channel})
    
    class events:
        def __init__(self, api):
            self._api = api
            self._handlers = {}
        
        def on(self, event_name):
            def decorator(func):
                self._handlers[event_name] = func
                self._api._request("events.subscribe", {"event": event_name})
                return func
            return decorator
        
        def subscribe(self, event_name, callback):
            self._handlers[event_name] = callback
            self._api._request("events.subscribe", {"event": event_name})
        
        def dispatch(self, event_name, data):
            if event_name in self._handlers:
                self._handlers[event_name](data)


def load_plugin(plugin_dir):
    """Load a Python plugin from directory."""
    manifest_path = os.path.join(plugin_dir, "manifest.json")
    if not os.path.exists(manifest_path):
        print(f"ERROR: No manifest.json in {plugin_dir}", file=sys.stderr)
        return None
    
    with open(manifest_path) as f:
        manifest = json.load(f)
    
    entrypoint = manifest.get("entrypoint", "main.py")
    entry_path = os.path.join(plugin_dir, entrypoint)
    
    if not os.path.exists(entry_path):
        print(f"ERROR: Entrypoint {entrypoint} not found", file=sys.stderr)
        return None
    
    return manifest, entry_path


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <pipe_path> <plugin_dir>", file=sys.stderr)
        sys.exit(1)
    
    pipe_path = sys.argv[1]
    plugin_dir = sys.argv[2]
    
    # Load plugin
    result = load_plugin(plugin_dir)
    if result is None:
        sys.exit(1)
    
    manifest, entry_path = result
    
    # Setup API
    api = TasiaAPI(lambda msg: None)  # Placeholder send
    
    # Execute the plugin
    try:
        with open(entry_path) as f:
            code = compile(f.read(), entry_path, 'exec')
        
        # Create module namespace with API
        ns = {
            "tasia": api,
            "__file__": entry_path,
            "__name__": manifest.get("id", "plugin"),
        }
        exec(code, ns)
        
        print(f"Plugin '{manifest.get('name', 'unknown')}' loaded successfully")
        
        # Main loop - listen for IPC messages
        # In real implementation, this would read from pipe/socket
        import time
        while True:
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("Plugin shutting down")
    except Exception as e:
        print(f"Plugin error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
