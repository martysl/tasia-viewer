# Next Action

## Current
1. ✅ Linux build works (v0.1.0 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix MSVC compiler not found.** ← NOW
4. ⏳ macOS build (blocked on Windows).

## Windows blocker
- **Real root cause**: `vswhere` is broken on GitHub Actions `windows-2022` runner. CMake uses vswhere to find the VS instance. When vswhere fails, CMake can't detect the compiler.
- **Fix 1** ✅: Changed guard from `VCToolsInstallDir` to `VSINSTALLDIR`. Verified working.
- **Fix 2** 🔄: Save/restore `cl.exe` bin dir in PATH around `autobuild source_environment`.
- **Fix 3** 🔄: Export `CMAKE_GENERATOR_INSTANCE=${VSINSTALLDIR}` as env var to bypass broken vswhere.
- **Next**: Push fixes, trigger Windows build, verify CMake finds C/CXX compiler.
