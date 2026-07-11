# Next Action

## Current
Both builds retriggered:
- Linux: `tasia_linux_stable_os` (after `llbugreport.cpp` setText fix)
- Windows: `tasia_windows_os_stable` (after MsQuic.cmake POST_BUILD fix + workflow cleanup)

## Next
1. Check build results.
2. If Linux passes → download artifact, runtime test portable mode + grid connection.
3. If Windows passes → download artifact, verify `msquic.dll` is in zip, runtime test.
4. If both pass → publish as new prerelease.

## Blockers
- Waiting for build results.
- `NECESSARY_SIGNING_TOKEN` not yet obtained for optional code signing.
