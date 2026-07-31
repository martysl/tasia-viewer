# Plugin Manifest Format

Every plugin must have a `manifest.json` file in its directory.

## Example

```json
{
    "id": "com.tasia.example.hello",
    "name": "Hello Tasia",
    "version": "1.0.0",
    "author": "Tasia",
    "description": "Example Tasia Viewer plugin",
    "entrypoint": "main.lua",
    "runtime": "lua",
    "api_version": 1,
    "permissions": [
        "viewer.notify",
        "chat.send"
    ]
}
```

## Fields

| Field        | Required | Description |
|-------------|----------|-------------|
| `id`        | Yes      | Reverse-domain plugin ID (e.g., `com.tasia.example.hello`) |
| `name`      | Yes      | Human-readable plugin name |
| `version`   | Yes      | Semantic version (x.y.z) |
| `author`    | No       | Plugin author name |
| `description` | No     | Short description |
| `entrypoint` | Yes     | Script filename relative to plugin directory |
| `runtime`   | Yes      | One of: `lua`, `javascript`, `python` |
| `api_version` | Yes    | Protocol API version (currently 1) |
| `permissions` | No     | Array of permission strings the plugin requests |

## Plugin ID Rules

- Must be reverse-domain notation: `com.example.plugin`
- Only alphanumeric characters and dots
- Maximum 128 characters

## Validation Rules

Manifests are validated on discovery. Rejection reasons:
- Missing ID
- Duplicate ID (across all discovered plugins)
- Invalid ID format
- Missing or invalid version
- Unsupported API version
- Invalid entrypoint path (path traversal detected)
- Invalid runtime name
