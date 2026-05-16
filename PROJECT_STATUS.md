# Project Status

## Current Phase
Linux viewer builds and runs with FMOD.
Windows CI build is the active task on `windows-build-test` branch.

## Build Status
| Platform | Build | Runtime |
|----------|-------|---------|
| Linux    | ✅ v0.1.0 | ✅ (basic login) |
| Windows  | ❌ MSVC compiler not found | - |
| macOS    | ⏳ blocked on Windows first | - |

## Completed Milestones
- v0.1.0 release (tagged) — Linux builds and runs with FMOD
- FMOD integration working (private deps pipeline)
- KDU removed
- Second Life grids removed from bundled defaults
- release branch created
- GitHub Actions: manual-only workflow, single-platform
- Grid Lock added (I-Grid Beta included, SL blocked programmatically)
- Version bumped to 8.0.1 (display version with GitHub run number)
- TasiaFeed and TasiaBugReport backend integration (PHP + DB)
- BugSplat removed, crash reporter reconfigured to generic HTTP endpoint

## Current Windows Blocker
**CMake cannot find C/CXX compiler** on GitHub Actions `windows-2022` runner.
- Root cause: `configure_firestorm.sh` calls `load_vsvars` which resets the MSVC dev environment that `ilammy/msvc-dev-cmd@v1` set up.
- Fix attempted: Guard `load_vsvars` behind `VCToolsInstallDir` check + save/restore MSVC env vars around `autobuild source_environment`.
- Build `25973284340` still failed: `VCToolsInstallDir` was empty at guard time despite save/restore.
- Added `[TASIA DEBUG]` logging to trace env state at script entry, after save, and after restore.
- **Next**: Push debug logging, trigger build, inspect log output.
