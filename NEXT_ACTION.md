# Next Action

## Current
1. ✅ Linux build works (v0.1.0 tagged).
2. ✅ Grid lock implemented, version bumped to 8.0.1.
3. ❌ **Windows CI build — fix MSVC compiler not found.** ← NOW
4. ⏳ macOS build (blocked on Windows).

## Windows blocker
- **Root cause**: `configure_firestorm.sh` calls `load_vsvars` which resets MSVC environment that `ilammy/msvc-dev-cmd` already set up.
- **Fix attempted**: Guard `load_vsvars` + save/restore MSVC env vars around `autobuild source_environment`.
- **Build result**: Guard still triggered (`VCToolsInstallDir` empty at guard time). Save/restore didn't help.
- **Added debug logging**: `[TASIA DEBUG]` lines trace `VCToolsInstallDir`, `VSINSTALLDIR`, `VCINSTALLDIR` at entry, save, and restore.
- **Next**: Push debug logging, trigger Windows build, read debug output to find root cause.
