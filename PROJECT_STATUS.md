# Project Status

## Current Phase
Linux viewer builds and runs with FMOD.
Windows CI build is the active task on `windows-build-test` branch.

## Build Status
| Platform | Build | Runtime |
|----------|-------|---------|
| Linux    | ✅ v8.0.1-39 | ✅ (basic login) |
| Windows  | ❌ Ninja build: MSQuic C files compiled as C++ (build 25992480046) | - |
| macOS    | ⏳ blocked on Windows first | - |

## Completed Milestones
- v0.1.0 release (tagged) — Linux builds and runs with FMOD
- FMOD integration working (private deps pipeline)
- KDU removed
- Second Life grids removed from bundled defaults
- GitHub Actions: manual-only workflow, single-platform
- Grid Lock added (I-Grid Beta included, SL blocked programmatically)
- Version bumped to 8.0.1 (display version with GitHub run number)
- TasiaFeed and TasiaBugReport backend integration (PHP + DB)
- BugSplat removed, crash reporter reconfigured to generic HTTP endpoint
- TasiaFeed crash fix: removed manual draw() call, use refreshControls()
- TasiaFeed upload URL fix: added .php extension
- TasiaFeed upload HTTP fix: postJsonAndSuspend instead of postAndSuspend
- TasiaFeed upload response fix: read parsed JSON keys directly

## Current Windows Blocker
**MSQuic C files are compiled as C++ on Windows Ninja builds** because global
Windows compile options force `/TP` for every source language.

### Root Cause
`vswhere` (VS instance discovery) is **broken** on GitHub Actions `windows-2022` runner.
CMake uses vswhere when using `-G "Visual Studio 17 2022"`, and if vswhere fails,
CMake cannot identify the compiler.

### Solution: Ninja generator
Switched to **Ninja** generator on Windows CI. Ninja does not require vswhere —
it finds `cl.exe` from PATH (set by `ilammy/msvc-dev-cmd@v1`).

### Problem: `--ninja` passthrough broken
`autobuild configure` consumes the `--ninja` flag and does **not** forward it to
`configure_firestorm.sh`. Build log showed `NINJA: false` even though `--ninja`
was passed in the workflow.

### Fix applied (commit 79dee2e63c)
Auto-detect Ninja on Windows: if `TARGET_PLATFORM == "windows"` and `ninja`
is available on PATH, automatically enable Ninja generator. This bypasses the
autobuild passthrough issue.

### New build result (build 25992060682 / #43)
- ✅ CMake configure succeeded with Ninja.
- ✅ Build entered Ninja phase.
- ❌ Failed at: `ninja: error: 'sharedlibs/llwebrtc.dll', needed by
  'newview/copy_touched.bat', missing and no known rule to make it`.

### Fix applied for build #43 failure
Declared `${SHARED_LIB_STAGING_DIR}/llwebrtc.dll` as a Windows byproduct of the
`llwebrtc` POST_BUILD copy command in `indra/llwebrtc/CMakeLists.txt`, so Ninja
can associate `sharedlibs/llwebrtc.dll` with the `llwebrtc` target.

### New build result (build 25992480046 / #44)
- ✅ llwebrtc byproduct issue fixed; build progressed further.
- ❌ Failed compiling MSQuic C source (`_deps/msquic-src/src/core/api.c`).
- Root cause: global Windows `/TP` option compiles `.c` files as C++, causing
  C++ conversion and syntax errors in MSQuic.

### Fix in progress for build #44 failure
- `indra/cmake/00-Common.cmake`: changed `/TP` to
  `$<$<COMPILE_LANGUAGE:CXX>:/TP>` so only C++ sources are forced to C++ mode.
- `.github/workflows/build-windows.yml`: added Windows-only workflow on
  `windows-build-test` to isolate Windows CI from Linux CI.

### Previous abandoned fixes
- `CMAKE_GENERATOR_INSTANCE` env var: CMake evaluates it before `-G` is
  processed, so it can't fix vswhere-based detection for VS generators.

## Next Steps
1. ✅ Auto-detect Ninja on Windows committed & pushed
2. ✅ Build #43 verified CMake configure succeeds with Ninja
3. ✅ Build #44 verified llwebrtc byproduct fix works
4. ⏳ Commit/push Windows-only workflow + MSQuic `/TP` fix
5. ⏳ Trigger `build-windows.yml`
6. ⏳ Verify entire Windows build completes
