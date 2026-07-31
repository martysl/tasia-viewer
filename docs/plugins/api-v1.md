# Plugin API v1 Reference

The plugin API uses JSON-RPC 2.0 style messages over the internal protocol (`tasia://plugin-api/v1`).

## Transport

Currently: JSON over internal pipes.
Future: MessagePack support planned via the transport abstraction layer.

## Method Reference

### viewer.getVersion

Returns the viewer version.

```json
// Request
{"jsonrpc": "2.0", "id": 1, "method": "viewer.getVersion", "params": {}}
// Response
{"jsonrpc": "2.0", "id": 1, "result": {"version": "8.0.1.78571", "build": "Tasia-Releasex64"}}
```

### viewer.getGrid

Returns current grid/region info.

### viewer.notify

Shows a notification to the user.

Params: `{ "message": "text" }`

Permission: `viewer.notify`

### viewer.openFloater

Opens a viewer floater by name.

Params: `{ "name": "plugin_manager" }`

Permission: `viewer.open_floater`

### chat.send

Sends a chat message.

Params: `{ "message": "Hello", "channel": 0 }`

Permission: `chat.send`

### chat.listen / chat.unlisten

Subscribe or unsubscribe from chat messages.

Permission: `chat.read`

### avatar.getSelf

Returns current avatar info: id, name, position.

Permission: `avatar.read`

### avatar.getPosition

Returns current avatar position.

Permission: `avatar.read`

### avatar.getNearby

Returns list of nearby avatars. (Stub - not yet fully implemented)

Permission: `avatar.nearby`

### world.getRegion

Returns current region name, host, and ID.

Permission: `world.read`

### world.teleport

Teleport to a location.

Params: `{ "destination": "http://grid.example.com:8002/region/128/128/50" }`

Permission: `world.teleport`

### camera.get

Returns current camera position.

Permission: `camera.read`

### storage.get / storage.set / storage.delete

Per-plugin key-value storage. Data persists between sessions.

Params (get): `{ "key": "mykey" }`
Params (set): `{ "key": "mykey", "value": "myvalue" }`

Permission: `storage.plugin`

### ui.showDialog

Shows an alert dialog.

Params: `{ "title": "Title", "message": "Message" }`

Permission: `ui.create`

### network.httpRequest

Makes an HTTP request. (Stub - not yet fully implemented)

Permission: `network.http`

### events.subscribe / events.unsubscribe

Subscribe to viewer events.

Params: `{ "event": "chat.received" }`

### tasia.mom.* (Mom API)

Mom avatar detection API. See Mom API documentation.

Permissions: `mom.read`, `mom.events`

## Event Reference

Events are delivered as JSON-RPC notifications:

```json
{"jsonrpc": "2.0", "method": "event", "params": {"name": "region.changed", "data": {...}}}
```

### viewer.started
Emitted when the viewer finishes initialization and plugin system is ready.

### viewer.shutdown
Emitted when the viewer is shutting down.

### login.started / login.completed
Login lifecycle events.

### region.changed
Data: `{ "region_name": "...", "region_id": "..." }`

### avatar.nearby / avatar.left
Avatar presence events.

### chat.received / chat.sent
Chat message events.
Data: `{ "message": "...", "sender": "...", "channel": 0 }`

### im.received
Instant message received. Requires `im.read` permission.

### inventory.changed
Inventory modification event.

### object.selected / object.touched
Object interaction events.

### teleport.started / teleport.completed
Teleport lifecycle events.

### mom.online / mom.offline / mom.entered_sim / mom.left_sim / mom.distance_changed
Mom avatar tracking events. Requires `mom.events` permission.

## Error Codes

| Code | Meaning |
|------|---------|
| -32700 | Parse error |
| -32600 | Invalid request |
| -32601 | Method not found |
| -32602 | Invalid params |
| -32603 | Internal error |
| 1 | Permission denied |
| 2 | Runtime error |
| 3 | Timeout |
| 4 | Not implemented |
| 5 | Rate limited |
| 6 | Message too large |
