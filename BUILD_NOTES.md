# Build Notes

## Build Environment
- **Platform**: Ubuntu 22.04 (GitHub Actions runner)
- **Compiler**: GCC 11+ 
- **Build system**: CMake + autobuild + Ninja
- **FMOD**: Required (2.03.07), private package in martysl/tasia-private-deps

## Known Build Issues

### GCC -Wmaybe-uninitialized (FIXED)
- **File**: `indra/newview/llvisualeffect.h:130` - `LLTweenableValueLerp<LLVector4>::m_StartTime`
- **Fix**: Added member initializers in constructor (m_StartTime(0.0), m_Duration(0.0), m_StartValue(), m_EndValue())
- **Status**: ✅ Fixed in commit e466846140

## Configuration
- Channel: `Tasia-Releasex64`
- `--fmodstudio` flag required
- No KDU

## Workflow
- Manual trigger only via GitHub Actions (`workflow_dispatch`)
- Single platform per run
- Build order: Linux → Windows → macOS

## 2026-05-17: TasiaFeed upload fixes

### Fix 1: Wrong HTTP method (postAndSuspend with string → implicit LLSD)
- **Symptom**: Viewer sends wrapped XML instead of raw JSON
- **Root cause**: `postAndSuspend` has no `std::string` overload. String body was implicitly converted to LLSD and sent as `application/llsd+xml` XML via `requestPostWithLLSD` → server received `<llsd><string>{json}</string></llsd>`
- **Fix**: Changed to `postJsonAndSuspend` which sends raw JSON via `HttpCoroJSONHandler`
- **Commits**: linux: 75d0d60276, windows-build-test: 43f6446404

### Fix 2: Wrong response handler (expected HTTP_RESULTS_RAW)
- **Symptom**: Even with proper JSON sending, "No response from server" still shows
- **Root cause**: `TasiaFeedUploadResponse` checked for `HTTP_RESULTS_RAW` ("raw") key, which is **only** set by `HttpCoroRawHandler`. But `postJsonAndSuspend` uses `HttpCoroJSONHandler` which parses JSON and returns keys directly in `aData` (no "raw" key). So every response hit the error path.
- **Fix**: Rewrote `TasiaFeedUploadResponse` to read the already-parsed JSON keys (`success`, `post_url`, `message`) directly from `aData` instead of going through `HTTP_RESULTS_RAW`.
- **Commits**: linux: 85493dbca7, windows-build-test: 9cb3609280

### Files changed
- `indra/newview/tasiafeedconnect.cpp`

## 2026-05-16: BugSplat removal / crash reporter reconfiguration

### Summary
Replaced the BugSplat crash reporting pipeline with a generic HTTP crash endpoint. All BugSplat-specific validation, fallback URL construction, and DB-name logic have been removed or relaxed.

### Changes

1. **`indra/linux_crash_logger/linux_crash_logger.cpp`** — Removed BugSplat fallback URL construction. The endpoint string is now used directly as-is (`std::string url = strEndpoint;`).

2. **`indra/newview/llappviewerlinux.cpp`** — Renamed `gBugsplatDB` → `gCrashReportURL`. Changed all `"BUGSPLAT"` log tags to `"CRASHREPORT"`. The validation block now reads `"CrashReportURL"` from `build_data.json`, accepts any non-empty URL (no domain/prefix check), and rejects empty values instead.

3. **`indra/newview/viewer_manifest.py`** — Changed `build_data.json` key from `"BugSplat DB"` to `"CrashReportURL"`. Validation now requires an `http://` or `https://` URL prefix instead of the old `tasia_` / specific-domain check.

4. **`scripts/configure_firestorm.sh`** — Hardcoded the crash endpoint to `https://apps.easierit.org/igrid/bugs/api/v1/report` instead of constructing a dynamic `tasia_<channel>` BugSplat DB name.

### Rationale
Eliminates the dependency on BugSplat's proprietary SDK/API and lets the crash reporter send dumps to any standard HTTP endpoint. The viewer-side code no longer hardcodes assumptions about which SaaS service processes the crashes.
