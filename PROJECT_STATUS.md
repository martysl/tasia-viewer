# Project Status

## Current Phase

Linux viewer builds and runs with FMOD.

Next work:
1. **Grid lock**: remove/block Second Life and add I-Grid Beta.
2. **Version/build identity**: internal 8.0.1, display 8.0.1.<GitHub run number>, show short commit SHA.
3. **Snapshot system**: replace Flickr/Primfeed with real TasiaFeed upload.
4. **TasiaFeed backend**: PHP + DB + WebDAV storage under apps.easierit.org/igrid/feed/.
5. **Bug reporting**: adapt BugSplat/crash reporter into real TasiaBugReport/TasiaCrash.
6. **Bug backend**: PHP + DB/admin under apps.easierit.org/igrid/bugs/.
7. **Branding cleanup** while keeping legal credits intact.
8. **Manual Linux build** and test.

## Build Status

| Platform | Build | Runtime |
|----------|-------|---------|
| Linux    | ✅ v0.1.0 | ✅ (basic login) |
| Windows  | ⏳ blocked on Linux first | - |
| macOS    | ⏳ blocked on Windows first | - |

## Completed Milestones

- v0.1.0 release (tagged) - Linux builds and runs with FMOD
- FMOD integration working (private deps pipeline)
- KDU removed
- Second Life grids removed from bundled defaults
- release branch created
- GitHub Actions: manual-only workflow, single-platform
- GCC -Wmaybe-uninitialized fixed (real fix, not suppression)
- **Grid Lock added**: I-Grid Beta included, SL grids blocked programmatically at all entry points, startup purge of existing SL grids
- **Version bumped to 8.0.1**: Display version 8.0.1.<GitHub run number>, commit SHA visible in About window
