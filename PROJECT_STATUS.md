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

### Root Cause (refined)
The problem is **not** `load_vsvars` — the error occurs whether `load_vsvars` runs or is skipped.
- `ilammy/msvc-dev-cmd@v1` log shows: `Not found with vswhere` — `vswhere` (VS instance discovery) is **broken** on this runner.
- CMake uses vswhere to find the VS instance when using `-G "Visual Studio 17 2022"`.
- If vswhere fails, CMake cannot identify the compiler even though the MSVC env vars are set.

### Fixes applied so far

**Fix 1 - Guard fix**: Changed `load_vsvars` guard from `VCToolsInstallDir` to `VSINSTALLDIR` (build `25973540095` confirmed it works).

**Fix 2 - PATH save/restore**: `autobuild source_environment` may remove MSVC bin directories from PATH. Added save/restore of `cl.exe` path.

**Fix 3 - CMAKE_GENERATOR_INSTANCE**: Set the environment variable `CMAKE_GENERATOR_INSTANCE` to `VSINSTALLDIR` to bypass broken vswhere and tell CMake directly which VS instance to use.

### Debug findings
- Build `25973424632` debug logging: `VCToolsInstallDir` empty at entry (not set by `msvc-dev-cmd`), `VSINSTALLDIR` and `VCINSTALLDIR` set, `cl.exe` in PATH.
- Build `25973540095`: Guard correctly skips `load_vsvars` but CMake still fails with same error.

### Next
Push fixes 2+3, trigger Windows build, verify CMake finds C/CXX compiler.
