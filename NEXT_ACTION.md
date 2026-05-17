# Next Action

## Current
1. ✅ Linux build works (v8.0.1-39 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix final link and ZIP artifact.** ← NOW
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
- **Build #45 result** ✅ MSQuic `.c` compile issue fixed; build progressed to
  viewer manifest copy.
- **Build #46 result** ✅ manifest/sharedlibs fix worked; build reached final
  `firestorm-bin.exe` link.
- **New blocker** ❌ final link misses Boost filesystem symbols referenced by
  colladadom.
- **Fix in progress**: link `ll::boost` into final viewer target and fix MSVC
  `/MAP` flag.
- **Packaging change**: Windows CI should create an unpacked ZIP artifact instead
  of an installer for now.
- **Workflow isolation in progress**: add `.github/workflows/build-windows.yml`
  so Windows CI is platform-specific on `windows-build-test`.
- **Next**: Commit/push final link + ZIP artifact fixes, trigger Windows CI.

## TasiaFeed (all fixed, verified in Linux pre-release v8.0.1-39)
- ✅ Upload URL: added `.php` extension
- ✅ Crash: removed manual `draw()`, use `refreshControls()`
- ✅ Upload HTTP: `postJsonAndSuspend` instead of `postAndSuspend`
- ✅ Upload response: read parsed JSON keys directly (no `HTTP_RESULTS_RAW`)
- ✅ Folder/list feed browsing, private visibility, owner dashboard deployed
