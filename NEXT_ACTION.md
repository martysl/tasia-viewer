# Next Action

## Current
1. ✅ Linux build works (v0.1.0 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix MSVC compiler not found.** ← NOW
4. ⏳ macOS build (blocked on Windows).

## Windows blocker
- **Root cause**: `configure_firestorm.sh` calls `load_vsvars` which resets MSVC environment that `ilammy/msvc-dev-cmd` already set up.
- **Fix**: Guard `load_vsvars` — skip it when `VCToolsInstallDir` is already set (MSVC env already loaded).
- **Next**: Push fix, trigger one Windows build, verify CMake finds C/CXX compiler.
