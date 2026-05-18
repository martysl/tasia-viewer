# Project Status

## Current Phase
Linux viewer builds and runs with FMOD.
Windows CI build is the active task on `windows-build-test` branch.

## Build Status
| Platform | Build | Runtime |
|----------|-------|---------|
| Linux    | ✅ v8.0.1-39 | ✅ (basic login) |
| Windows  | ❌ Final link: Boost filesystem unresolved from colladadom (#48 = 25996616402, #49 = 25998856384 running) | - |
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

## Current Windows Status
**Windows CI now builds successfully and uploads the ZIP artifact.**

### 2026-05-18 deeper finding
- Build `26025086918` proved the issue is not just missing Boost 1.86.
- When Boost.Filesystem was forced to `/Zc:wchar_t-`, colladadom-style symbols
  changed, but viewer objects then failed on native `wchar_t` symbols.
- Therefore one Boost build cannot satisfy both ABIs:
  - prebuilt `libcollada14dom23-s.lib` expects legacy `unsigned short` mangling
    (`/Zc:wchar_t-`)
  - viewer code expects native `wchar_t` mangling (`/Zc:wchar_t`)
- Fix now applied on `windows-build-test`:
  - reverted Boost `/Zc:wchar_t-` forcing
  - added `indra/newview/llboostfilesystemcompat.cpp`, a Windows-only shim that
    provides the two legacy unsigned-short `path_traits::convert` symbols and
    forwards to native Boost 1.86 functions
  - added the shim to Windows viewer sources

### Last attempted
- Commit `09603eb7c1`: `Fix Windows symbol packaging build_data path for Ninja`
- Dispatched cached run `26032071141` with `clean_build=false`, `probe_only=false`.
- Result: ✅ success. Artifact `Tasia-Viewer-Windows-FMOD` uploaded
  (636,116,759 bytes).
- Published GitHub prerelease `v8.0.1-15-windows` with asset
  `Tasia-Viewer-Windows-FMOD.zip`.

### Root Cause (real one this time)
**ABI mismatch**: prebuilt `libcollada14dom23-s.lib` was compiled against
**Boost 1.86**, but FetchContent built **Boost 1.87**. The `path_traits::convert`
symbol was renamed/removed in Boost 1.87's filesystem ABI changes.

### Fix (build #52 and #53)
- `indra/cmake/Boost.cmake`: use **Boost 1.86.0** on Windows (via FetchContent)
  to match the prebuilt colladadom. Keep 1.87.0 on other platforms.
- `indra/cmake/LLPrimitive.cmake`: adopted Misfitz fix (commit 7bba6b835f) —
  use `find_library` + link `ll::boost` on **all** platforms (was missing on
  Windows).

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

### Fix applied for build #46 failure (was not enough)
- `indra/newview/CMakeLists.txt`: explicitly link `ll::boost` into final viewer
  target so Boost filesystem is present after colladadom.
- Fixed MSVC release map flag to `/MAP:secondlife-bin.MAP`.
- Windows CI now disables installer packaging and creates a ZIP from
  `LOCAL_DIST_DIR` unpacked output instead.

### New build result (build #48 = 25996616402)
- ✅ Build reached 1638/1640 objects — almost complete!
- ❌ Failed at final link: same `boost::filesystem::detail::path_traits::convert` 
  unresolved symbols from `libcollada14dom23-s.lib`.
- Reason: `ll::boost` in viewer target didn't solve it because the dependency
  must be declared at the `ll::colladadom` level.

### Real root cause found (commit 7d2133b3ab)
- `indra/cmake/LLPrimitive.cmake` line 44: on Windows, `ll::colladadom` was
  missing `ll::boost` in its INTERFACE link libraries. Linux and Darwin had it.
- The prebuilt `libcollada14dom23-s.lib` references `boost::filesystem` symbols,
  so `ll::boost` must be linked at the `ll::colladadom` level so CMake orders 
  dependencies correctly.
- **Fix**: added `ll::boost` to the Windows `target_link_libraries` for
  `ll::colladadom`, matching Linux and Darwin.

### Previous abandoned fixes
- `CMAKE_GENERATOR_INSTANCE` env var: CMake evaluates it before `-G` is
  processed, so it can't fix vswhere-based detection for VS generators.

## Build Rules (set in stone)
- **Linux builds** → use `linux` branch. Trigger via `build-tasia.yml` with `target=linux`.
- **Windows builds** → use `windows-build-test` branch. Trigger via `build-windows.yml` (file lives on `linux` branch for dispatch, but checks out `windows-build-test` code).
- **Never push Windows viewer code to `linux`**. Only CI workflow files (`build-windows.yml`) may live on `linux` for dispatch purposes.

## Latest Windows Build
- Build #48 (25996616402): ❌ boost filesystem unresolved.
- Build #49 (25998856384): ❌ same — `ll::boost` in colladadom didn't help (1.87 vs 1.86 ABI).
- Build #50 (26000378886): ❌ prebuilt boost 1.86 lib not found on runner.
- Build #51 (26000598938): ❌ Boost::filesystem direct link still 1.87 ABI.
- Build #52 (26000669561): ❌ same — Misfitz's `find_library` + `ll::boost` fix alone not enough.
- Build #53 (26002326106): ✅ probe passed — Boost 1.86 configured.
- Build `26008774063`: ❌ same colladadom `path_traits::convert` unresolved.
- Build `26025086918`: ❌ after forcing Boost `/Zc:wchar_t-`, viewer native
  `wchar_t` Boost.Filesystem symbols failed instead — confirmed dual ABI issue.
- Build `26028393584`: ❌ link succeeded, failed symbol packaging because
  Ninja single-config used `configuration='.'` and `build_data.json` was not at
  `./build_data.json`.
- Build `26032071141`: ✅ success using cache after `fs_viewer_manifest.py`
  fallback path fix. ZIP artifact uploaded: `Tasia-Viewer-Windows-FMOD`.
- Prerelease `v8.0.1-15-windows`: ✅ created from commit `09603eb7c1` with
  `Tasia-Viewer-Windows-FMOD.zip` asset.

## Next Steps
1-6. ✅ All Ninja/build fixes applied.
7-11. ✅ All boost ABI attempts documented.
12. ✅ **Build 26032071141** succeeded with Boost 1.86 + Windows ABI shim +
    Ninja build_data fallback.
13. ✅ Windows prerelease published: `v8.0.1-15-windows`.
14. Next: download/test the Windows ZIP artifact.
