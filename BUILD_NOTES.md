# Build Notes

## Build Environment
- **Platform**: Ubuntu 22.04 (GitHub Actions runner)
- **Compiler**: GCC 11+ (Linux), MSVC v143 (Windows)
- **Build system**: CMake + autobuild + Ninja (Linux) / MSBuild (Windows)
- **FMOD**: Required (2.03.07), private package in martysl/tasia-private-deps

## Workflow
- Manual trigger only via GitHub Actions (`workflow_dispatch`)
- Single platform per run
- Build order: Linux → Windows → macOS

## 2026-05-16: Windows CI — MSVC compiler not found fix

### Root cause
The `configure_firestorm.sh` script unconditionally calls `load_vsvars` on Windows (line 370).
In CI environments where MSVC is already set up (via `ilammy/msvc-dev-cmd@v1` or `VsDevCmd.bat`),
`load_vsvars` re-initializes the Visual Studio environment, overriding/corrupting the
correct PATH, INCLUDE, and LIB variables. CMake then can't find the compiler.

### Fix
**File**: `scripts/configure_firestorm.sh`
**Change**: Added a guard around `load_vsvars` — check if `VCToolsInstallDir` is already set.
If MSVC env is already loaded, skip `load_vsvars` and preserve the existing environment.

```bash
if [ -z "${VCToolsInstallDir:-}" ]; then
    load_vsvars
else
    echo "MSVC already loaded, skipping load_vsvars"
fi
```

### Workflow state (windows-build-test branch)
- Uses `ilammy/msvc-dev-cmd@v1` with `arch: x64, vsversion: 2022`
- Verify MSVC tools step confirms `cl.exe` and `cmake` are found
- Configure step uses `autobuild configure` with `--fmodstudio --package`
- Linux/macOS configure step unchanged

## 2026-05-18: Windows final link — Boost.Filesystem/colladadom ABI

### Root cause
Windows final link reaches `firestorm-bin.exe` but `libcollada14dom23-s.lib`
references Boost.Filesystem `path_traits::convert` symbols using legacy
`unsigned short` mangling (`/Zc:wchar_t-`). Viewer code and FetchContent Boost
1.86 use native `wchar_t` mangling (`/Zc:wchar_t`).

Forcing Boost.Filesystem to `/Zc:wchar_t-` resolves the colladadom side but
breaks viewer object references to native `wchar_t` Boost symbols. Therefore the
correct fix is not changing Boost's ABI globally.

### Current fix
- Keep Boost 1.86 native for viewer code.
- Add `indra/newview/llboostfilesystemcompat.cpp` as a Windows-only shim that
  provides colladadom's legacy unsigned-short `path_traits::convert` entry
  points and forwards to native Boost.Filesystem functions.
- Run `26028393584` confirmed the final link fix; it then failed later in
  Windows symbol packaging because Ninja single-config passed
  `configuration='.'` and `fs_viewer_manifest.py` looked for
  `./build_data.json`.
- `indra/newview/fs_viewer_manifest.py` now falls back to `dest/build_data.json`
  and the parent build directory.
- Run `26032071141` succeeded using restored Windows cache and uploaded
  `Tasia-Viewer-Windows-FMOD` (636,116,759 bytes).
