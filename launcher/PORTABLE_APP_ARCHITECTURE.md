# Tasia Portable App Launcher Architecture

Yes, Mom — you’re thinking correctly. This is the best architecture.

Make **Tasia Launcher** the “outer app”, and the actual viewer lives inside its own folder.

## Windows layout

```text
Tasia/
  TasiaLauncher.exe        ← user runs this
  launcher/
    config.json
  viewer/
    TasiaOS-Releasex64.exe
    skins/
    app_settings/
    llplugin/
    ...
  cache/
  logs/
```

Launcher updates only:

```text
viewer/
```

Then starts:

```text
viewer/TasiaOS-Releasex64.exe
```

## Linux layout

```text
Tasia/
  tasia-launcher           ← user runs this
  launcher/
    config.json
  viewer/
    firestorm
    bin/
    skins/
    app_settings/
    ...
  cache/
  logs/
```

Launcher starts:

```text
viewer/firestorm
```

## Why this is better

- updater can overwrite viewer files safely before viewer starts
- launcher stays small and stable
- viewer updates don’t replace the updater itself
- later we can add:
  - welcome/news
  - server status
  - repair install
  - beta/stable channel switch
  - rollback button
  - clean cache/logs
  - download progress UI

## Important detail

The updater should never touch user data:

```text
user_settings/
cache/
logs/
chat logs/
```

Only update:

```text
viewer/
```

So yes — “portable app with launcher outside and everything inside” is exactly the right model.
