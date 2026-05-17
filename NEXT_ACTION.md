# Next Action

## Current
1. ✅ Linux build works (v8.0.1-39 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix MSVC compiler not found.** ← NOW
4. ⏳ macOS build (blocked on Windows).

## Windows blocker
- **Real root cause**: `vswhere` is broken on GitHub Actions `windows-2022`
  runner. CMake uses vswhere for VS generator detection.
- **Solution**: Use **Ninja** generator + `cl.exe` from PATH (set by
  `msvc-dev-cmd`). Ninja doesn't need vswhere.
- **Problem**: `autobuild configure` strips `--ninja` flag on passthrough.
- **Fix** ✅ Auto-detect Ninja on Windows when `ninja` is in PATH
  (commit `79dee2e63c`, branch `windows-build-test`).
- **Build #43 result** ✅ CMake configure succeeded with Ninja; build reached
  Ninja phase.
- **New blocker** ❌ Ninja failed because `sharedlibs/llwebrtc.dll` is a
  POST_BUILD copy output but was not declared as a byproduct.
- **Fix in progress**: declare `${SHARED_LIB_STAGING_DIR}/llwebrtc.dll` as a
  Windows byproduct in `indra/llwebrtc/CMakeLists.txt`.
- **Next**: Commit/push llwebrtc Ninja byproduct fix, trigger Windows CI build.

## TasiaFeed (all fixed, verified in Linux pre-release v8.0.1-39)
- ✅ Upload URL: added `.php` extension
- ✅ Crash: removed manual `draw()`, use `refreshControls()`
- ✅ Upload HTTP: `postJsonAndSuspend` instead of `postAndSuspend`
- ✅ Upload response: read parsed JSON keys directly (no `HTTP_RESULTS_RAW`)
