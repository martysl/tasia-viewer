# Next Action

## Current
1. ✅ Linux build works (v8.0.1-39 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix Windows Ninja/MSQuic compile.** ← NOW
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
- **Build #44 result** ✅ llwebrtc byproduct issue fixed; build progressed to
  real C/C++ compilation.
- **New blocker** ❌ MSQuic `.c` source is compiled with `/TP` as C++, causing
  C++ conversion/syntax errors.
- **Fix in progress**: scope `/TP` to C++ sources only in
  `indra/cmake/00-Common.cmake`.
- **Workflow isolation in progress**: add `.github/workflows/build-windows.yml`
  so Windows CI is platform-specific on `windows-build-test`.
- **Next**: Commit/push Windows-only workflow + `/TP` fix, trigger Windows CI.

## TasiaFeed (all fixed, verified in Linux pre-release v8.0.1-39)
- ✅ Upload URL: added `.php` extension
- ✅ Crash: removed manual `draw()`, use `refreshControls()`
- ✅ Upload HTTP: `postJsonAndSuspend` instead of `postAndSuspend`
- ✅ Upload response: read parsed JSON keys directly (no `HTTP_RESULTS_RAW`)
