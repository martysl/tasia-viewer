# Project Status

## 2026-07-11 — Stable branch rebuild

### Branches
- `tasia_linux_stable_os` — Linux stable, manual-dispatch only
- `tasia_windows_os_stable` — Windows stable, manual-dispatch only, code signing workflow ready

### What is done (both branches)
- **Strict portable mode**: `portable-data/` auto-created next to binary. ALL data (cache, settings, logs, credentials) stays beside the executable. Works for any copy: `tasia1/` gets `tasia1/portable-data/`, `tasia2/` gets its own.
- **All grid blocking removed**: Connect to any grid (SL, OpenSim, I-Grid, Aurora, etc.)
- **OpenSSL/quictls verified**: No Schannel anywhere. Linux builds MsQuic from source (static). Windows uses Microsoft-signed pre-built DLL (v2.4.10).
- **TasiaGuard + bug report**: Self-service unban via HTTP API, bug report submission from viewer menu.
- **DAE mesh export**: Rigged meshes with skin weights, LLVector4a fixes.
- **Havok error suppressed**: No more missing Havok warning on OpenSim.
- **Looking Glass theme**: Semi-transparent Aero style, Family Grids branding, animated boot splash.
- **New Tasia cyan/blue logo icons**: All replaced.
- **2GB RAM / 2048MB VRAM defaults**.
- **Windows clipboard image upload**.
- **Linux clipboard image upload** (LLProcess, WebP, PNG normalize, resize).
- **Windows DPAPI protected password store**.
- **SSL certificate chain fixes**.
- **TasiaFeed owner page**.

### MsQuic / Windows Defender fix
- Windows builds no longer compile MsQuic/quictls from source.
- Downloads pre-built `msquic.dll` from Microsoft GitHub releases (v2.4.10).
- DLL is Microsoft-signed — no Defender false positives.
- No more NASM/Perl required on Windows CI.
- Linux still builds MsQuic from source (static, v2.5.7) — no Defender issues.

### Build status
- **Linux build**: Retriggered after `llbugreport.cpp` setText fix.
- **Windows build**: Retriggered after MsQuic.cmake POST_BUILD fix + workflow cleanup.

### What was last attempted
- Cherry-picked features from HEAD (`tasia_linux_sl`) one by one to stable branches.
- Self-diagnostic code check caught: `llbugreport.cpp` missing from Linux sources (moved from Windows-only to main), `LLSD::String(msg)` no-op, dead `registerTasiaGuardFloater()`.
- Build caught: `llbugreport.cpp` `setText("")` type mismatch (needs `std::string()`).
- Build caught: `add_custom_command(TARGET ... POST_BUILD)` on imported target (removed, moved to workflow step).

### What is broken
- Waiting for build results on both platforms.

### What must not be changed
- Do not revert to Schannel on Windows (confirmed broken on Windows 10).
- Do not re-add grid blocking.
- Do not add TasiaCrypt/IMs yet.
- Do not expose API tokens or secrets.

### Next exact action
- Check build results when they complete.
- If Linux passes → runtime test.
- If Windows passes → runtime test + check msquic.dll is in output zip.

## Previous releases
- `v8.0.1.78497`: Last prerelease with all features.
- `v8.0.1.78484`: Windows/SSL fixes, CA bundle patch.
