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
