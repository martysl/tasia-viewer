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
