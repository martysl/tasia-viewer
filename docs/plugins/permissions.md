# Plugin Permissions

Plugin permissions control what APIs a plugin can access. The permission system is designed with least-privilege principles.

## Permission List

| Permission | Description | High Risk |
|-----------|-------------|-----------|
| `viewer.notify` | Show notifications | No |
| `viewer.open_floater` | Open floaters | No |
| `chat.read` | Read public chat | No |
| `chat.send` | Send chat messages | No |
| `im.read` | Read private IM | **Yes** |
| `im.send` | Send private IM | **Yes** |
| `avatar.read` | Read avatar info | No |
| `avatar.nearby` | Track nearby avatars | No |
| `inventory.read` | Read inventory | No |
| `inventory.modify` | Modify inventory | **Yes** |
| `camera.read` | Read camera state | No |
| `camera.control` | Control camera | **Yes** |
| `world.read` | Read world info | No |
| `world.teleport` | Teleport | **Yes** |
| `network.http` | HTTP requests | **Yes** |
| `storage.plugin` | Plugin storage | No |
| `ui.create` | Create UI | **Yes** |
| `clipboard.read` | Read clipboard | **Yes** |
| `clipboard.write` | Write clipboard | No |
| `filesystem.plugin` | Access plugin files | No |
| `filesystem.external` | External files | **Yes** |
| `native.execute` | Native code/DLLs | **Yes** |
| `process.execute` | External processes | **Yes** |
| `microphone` | Microphone access | **Yes** |
| `mom.read` | Mom avatar status | No |
| `mom.events` | Mom avatar events | No |

## Rules

1. **No automatic grants**: Plugins get no sensitive permissions automatically.
2. **Permission dialog**: Shown on first enable, listing all requested permissions.
3. **Individual grant/revoke**: Permissions can be managed individually.
4. **Version tracking**: When a plugin updates, permissions must be re-granted.
5. **No self-editing**: A plugin cannot edit its own permission grants.
6. **High-risk warnings**: Marked with yellow warning in the permission dialog.
7. **IM privacy**: Private IM access requires explicit separate permission.
8. **External filesystem, native/process execution**: Always marked high risk.
9. **Native plugins**: Disabled by default, require compile-time option.

## Storage

Permissions are stored per plugin ID and version in:
- Linux: `~/.config/tasia/plugin_permissions.xml`
- Windows: `%APPDATA%/Tasia/plugin_permissions.xml`
