# Project Status

## Current Phase
Linux viewer builds and runs with FMOD.
Windows CI build is the active task on `windows-build-test` branch.

## Build Status
| Platform | Build | Runtime |
|----------|-------|---------|
| Linux    | ✅ v8.0.1-39 | ✅ (basic login) |
| Windows  | ❌ Final link: Boost filesystem unresolved from colladadom (build 25993589573) | - |
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
**Windows reaches final `firestorm-bin.exe` link but fails resolving Boost
filesystem symbols from `libcollada14dom23-s.lib`.**

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

### New build result (build 25992846062 / #45)
- ✅ MSQuic C compile issue fixed; build progressed further.
- ✅ `llwebrtc.dll` linked successfully.
- ❌ Failed during `viewer_manifest.py --actions=copy` because shared DLLs were
  staged in `build-vc170-64/sharedlibs/`, while the manifest searched
  `build-vc170-64/sharedlibs/Release`.

### Fix in progress for build #45 failure
- `indra/newview/viewer_manifest.py`: when `configuration` is `.` (single-config
  Ninja) and `sharedlibs/<buildtype>` does not exist, use `sharedlibs/` as the
  staging path.

### New build result (build 25993589573 / #46)
- ✅ Manifest/sharedlibs fix worked; build progressed to final executable link.
- ❌ Failed linking `firestorm-bin.exe` with unresolved Boost filesystem symbols
  referenced by `libcollada14dom23-s.lib`.
- Warning found: malformed `/MAP"secondlife-bin.MAP"` becomes `/MAPsecondlife-bin.MAP` under Ninja/MSVC.

### Fix in progress for build #46 failure
- `indra/newview/CMakeLists.txt`: explicitly link `ll::boost` into final viewer
  target so Boost filesystem is present after colladadom.
- Fixed MSVC release map flag to `/MAP:secondlife-bin.MAP`.
- Windows CI now disables installer packaging and creates a ZIP from
  `LOCAL_DIST_DIR` unpacked output instead.

### Previous abandoned fixes
- `CMAKE_GENERATOR_INSTANCE` env var: CMake evaluates it before `-G` is
  processed, so it can't fix vswhere-based detection for VS generators.

## Build Rules (set in stone)
- **Linux builds** → use `linux` branch. Trigger via `build-tasia.yml` with `target=linux`.
- **Windows builds** → use `windows-build-test` branch. Trigger via `build-windows.yml` (file lives on `linux` branch for dispatch, but checks out `windows-build-test` code).
- **Never push Windows viewer code to `linux`**. Only CI workflow files (`build-windows.yml`) may live on `linux` for dispatch purposes.

## Latest Windows Build
- Build #48 (25996616402) triggered from `linux` branch via `build-windows.yml`.
- Workflow checks out `ref: windows-build-test` so code comes from the right branch.
- Produces ZIP artifact `Tasia-Viewer-Windows-FMOD.zip`.

## Next Steps
1. ✅ Auto-detect Ninja on Windows committed & pushed
2. ✅ Build #43 verified CMake configure succeeds with Ninja
3. ✅ Build #44 verified llwebrtc byproduct fix works
4. ✅ Build #45 verified MSQuic `/TP` fix works
5. ✅ Windows Ninja manifest sharedlibs path fix committed & pushed
6. ✅ Final link + Windows ZIP artifact fixes committed & pushed
7. ✅ Build #48 triggered — waiting for result
8. ⏳ Verify ZIP artifact is produced
