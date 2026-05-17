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
- **Fix** ✅ Auto-detect Ninja on Windows when `ninja` is in PATH.
- **Build fixes applied**:
  - llwebrtc DLL as Ninja byproduct ✅
  - MSQuic C files no longer forced as C++ ✅
  - Viewer manifest Ninja single-config sharedlibs fallback ✅
  - ✅ ~~Explicit `ll::boost` link for final target~~ → **Real fix**: `ll::boost` in `ll::colladadom`
  - MSVC `/MAP` flag syntax fixed ✅
  - ZIP artifact instead of installer ✅
- **Workflow**: `build-windows.yml` lives on `linux` branch (for dispatch), but
  checks out `ref: windows-build-test` so build uses Windows code.
- **Build #48 (25996616402)**: ❌ succeeded 1638/1640 objects, failed at final link (boost filesystem)
- **Build #49 (25998856384)**: ❌ same error — `ll::boost` via FetchContent Boost 1.87 doesn't help (ABI changed)
- **Real root cause**: colladadom prebuilt `libcollada14dom23-s.lib` compiled against **Boost 1.86**, but we link **Boost 1.87** via FetchContent. `boost::filesystem::detail::path_traits::convert` was changed/removed in 1.87.
- **Real fix**: Link `libboost_filesystem-vc143-mt-x64-1_86.lib` (prebuilt 1.86) directly into `ll::colladadom` on Windows, not the FetchContent 1.87 version.
- **Build #50 (26000378886)**: 🚀 running with real fix — prebuilt Boost 1.86 filesystem linked directly
- **Next**: Wait for build #50 result. Verify ZIP artifact.

## Branch Rules (set in stone)
- **Linux builds** → `linux` branch, `build-tasia.yml` with `target=linux`.
- **Windows builds** → `windows-build-test` branch, `build-windows.yml` dispatch.
- **Never push Windows viewer code to `linux`**. CI workflow files only.

## TasiaFeed (all fixed, verified in Linux pre-release v8.0.1-39)
- ✅ Upload URL: added `.php` extension
- ✅ Crash: removed manual `draw()`, use `refreshControls()`
- ✅ Upload HTTP: `postJsonAndSuspend` instead of `postAndSuspend`
- ✅ Upload response: read parsed JSON keys directly (no `HTTP_RESULTS_RAW`)
- ✅ Folder/list feed browsing, private visibility, owner dashboard deployed
