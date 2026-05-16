# Next Action

## Current
1. ✅ Linux build works (v0.1.0 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix MSVC compiler not found.** ← NOW
4. ⏳ macOS build (blocked on Windows).

## Windows blocker
- **Root cause**: `configure_firestorm.sh` calls `load_vsvars` which resets MSVC environment that `ilammy/msvc-dev-cmd` already set up.
- **Bug in guard**: Was checking `VCToolsInstallDir` but `ilammy/msvc-dev-cmd@v1` does NOT set this var. It sets `VSINSTALLDIR`.
- **Fix**: Changed guard to check `VSINSTALLDIR` instead. Also keeps save/restore of `VSINSTALLDIR`/`VCINSTALLDIR` around `autobuild source_environment`.
- **Next**: Push fix, trigger Windows build, verify CMake finds C/CXX compiler.
