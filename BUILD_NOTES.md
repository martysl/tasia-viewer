# Build Notes

## 2026-05-25: Profile remote badge image loading

- Branch/worktree: `feature/tasia-tag-badge-fix` at `/mnt/a/2026/tasia-tag-badge-fix`.
- Changed profile remote badge loading away from `LLViewerTextureManager::getFetchedTextureFromUrl(...)`.
- New flow:
  - coroutine downloads raw image bytes with redirects enabled
  - timeout/transfer timeout: 60 seconds
  - retries: 0
  - range request/max accepted body: 10 MiB
  - content type is cleaned from response headers, with URL extension fallback
  - image is decoded with `gTextureList.getImageFromMemory(...)`
  - `LLThumbnailCtrl::setTexture(...)` displays the decoded in-memory texture
- Fallback remains `Profile_Badge_Team` if fetch/decode fails.

### Focused checks
- `git diff --check`: passed.
- Static review: handle lifetime, stale URL response guard, redirect/range support checked.

## 2026-05-24: Remote Tasia user config

- New files:
  - `indra/newview/lltasia_user_config.h`
  - `indra/newview/lltasia_user_config.cpp`
- Settings:
  - `TasiaRemoteUserConfigEnabled` default true
  - `TasiaRemoteUserConfigURL` default `https://i.let-us.cyou/hg/config.json`
  - `TasiaRemoteUserConfigTimeout` default 5 seconds
  - `TasiaRemoteUserConfigMaxBytes` default 262144
- Startup behavior:
  - Fetch is non-blocking via coroutine.
  - Fetch is one-shot per viewer run.
  - Failures only log a warning and do not block login.
- Rendering behavior:
  - Nametag adds custom title line from `custom_title`, `title`, `rank_title`, or fallback `badge_name`.
  - Nametag title color accepts `tag_color` or `color` as `#RRGGBB`/`#RRGGBBAA`.
  - Profile account area shows badge/profile text and tooltip; profile uses existing local badge icon slot.
- Welcome behavior:
  - Default changed from `welcome.txt` to `welcome.php`.
  - Existing persisted old URL is mapped to `welcome.php` at runtime.
  - First usable line is used; no randomization.
  - Fetch resets per visible login/teleport progress screen.

### Focused checks
- `git diff --check`: passed.
- `indra/newview/app_settings/settings.xml` XML parse: passed.
- Strict conflict-marker scan in `indra/newview` cpp/h/xml: passed.

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

## 2026-05-18: GIPHY/welcome/loading feature branch

### Generated GIPHY key support
- Branch: `feature/tasia-giphy-welcome-loading-linux`
- Build-time environment variable: `TASIA_GIPHY_API_KEY`
- Generator: `scripts/generate_tasia_giphy_key.py`
- Generated ignored files:
  - `indra/newview/lltasia_giphy_key.generated.h`
  - `indra/newview/lltasia_giphy_key.generated.cpp`
- Runtime accessor:
  - `indra/newview/lltasia_giphy_key.h`
  - `indra/newview/lltasia_giphy_key.cpp`

### Runtime key priority
1. `TasiaGiphyAPIKey` user setting
2. Generated obfuscated build-time fallback
3. Empty/not configured

### Verification run
- `env -u TASIA_GIPHY_API_KEY python3 scripts/generate_tasia_giphy_key.py --header indra/newview/lltasia_giphy_key.generated.h --source indra/newview/lltasia_giphy_key.generated.cpp`
- Fake-key generator test under `/tmp/opencode`: passed, plaintext fake key not present in generated source.
- `python3 -m py_compile scripts/generate_tasia_giphy_key.py`: passed.
- XML parse of `indra/newview/app_settings/settings.xml`: passed.
- `git diff --check`: passed.

### Full build status
- No full Linux build has been run yet after these feature edits.

### Welcome text client
- Files added:
  - `indra/newview/lltasia_welcome_client.h`
  - `indra/newview/lltasia_welcome_client.cpp`
- `LLProgressView` now requests one random usable line during startup loading.
- Fetch settings:
  - `TasiaWelcomeURL`
  - `TasiaWelcomeURLTimeout`
  - `TasiaWelcomeMaxBytes`
- Failure, timeout, empty body, or no valid line leaves the existing server/grid progress message unchanged.
- Late responses after startup completion are ignored.

### GIPHY API client
- Files added:
  - `indra/newview/llgiphyclient.h`
  - `indra/newview/llgiphyclient.cpp`
- Supports:
  - search endpoint
  - trending endpoint
  - rating from `TasiaGiphyRating`
  - result parsing for page URL, preview GIF, fixed-width GIF, downsized GIF, and original GIF
  - `GIPHY is not configured.` fallback when no runtime or generated key is available
- The client does not log request URLs, because those contain the API key query parameter.

### GIPHY picker floater
- Files added:
  - `indra/newview/llfloatergiphypicker.h`
  - `indra/newview/llfloatergiphypicker.cpp`
  - `indra/newview/skins/default/xui/en/floater_giphy_picker.xml`
- Registered floater name: `giphy_picker`
- Current behavior:
  - loads trending GIFs on open
  - supports search button and trending button
  - lists result title and normal GIPHY page URL
  - `Use Selected` invokes an optional callback or copies the selected URL to clipboard
  - includes `Powered by GIPHY` text
- Nearby chat wiring:
  - Added `GIF` button to `indra/newview/skins/default/xui/en/floater_fs_nearby_chat.xml`
  - Button opens `LLFloaterGiphyPicker`
  - Selected GIF sends the normal GIPHY page URL through `FSFloaterNearbyChat::sendChatFromViewer(...)`
  - Current whisper/say/shout selection is respected

### GIPHY chat previews
- Implemented in `indra/newview/fschathistory.cpp`.
- Controlled by `TasiaAnimatedGifChatPreview`.
- Current preview is a safe local card:
  - detects supported `giphy.com`, `media.giphy.com`, and `*.giphy.com` URL forms
  - shows `GIF preview`, canonical GIPHY page URL, `Powered by GIPHY`, and an `Open GIF` button
  - does not alter the sent chat payload
  - skips plain-text chat history and loaded chat logs to avoid mass widget creation
- Full inline animated thumbnail rendering is not implemented yet.
- Direct image URL previews are also implemented in `FSChatHistory`.
- Controlled by `TasiaImageChatPreview`.
- Supported direct image extensions:
  - `.png`
  - `.jpg`
  - `.jpeg`
  - `.gif`
  - `.webp`
  - `.bmp`
  - `.apng`
- Active nearby chat and Firestorm IM both use `FSChatHistory`; legacy `LLChatHistory` is currently disabled by `#if 0`.
- YouTube embeds are implemented in `FSChatHistory`.
- Controlled by `TasiaYouTubeChatPreview` (default true).
- Supported URL forms include `youtube.com/watch?v=...`, `youtube.com/embed/...`, `youtube.com/shorts/...`, `youtube.com/live/...`, and `youtu.be/...`.
- Chat sends and stores the normal URL; Tasia Viewer renders the local embed card.

### Loading panel branding / YouTube
- `panel_progress.xml` now says `Tasia Viewer uses` and includes `Powered by GIPHY`.
- Optional YouTube loading media is implemented in `LLProgressView`.
- Controlled by:
  - `TasiaLoadingYouTubeEnabled` (default false)
  - `TasiaLoadingYouTubeURL` (default empty)
- Accepted URL forms include `youtube.com/watch?v=...`, `youtube.com/embed/...`, `youtube.com/shorts/...`, `youtube.com/live/...`, and `youtu.be/...`.
- The URL is converted to a muted autoplay embed URL locally; the configured URL is not logged.
- Loading YouTube is not the default behavior; chat/IM YouTube embeds are the default behavior.

### GitHub Actions readiness
- Linux workflow `.github/workflows/build-tasia.yml` passes `secrets.TASIA_GIPHY_API_KEY` into configure.
- Build should still configure if `TASIA_GIPHY_API_KEY` is missing; generated fallback key will be empty and runtime GIPHY picker will report `GIPHY is not configured.` unless `TasiaGiphyAPIKey` is set by the user.
- Existing `FMOD_DEPS_TOKEN` secret remains required for FMOD dependency download.
- Generated GIPHY files are ignored and must not be committed.

### 2026-05-18: Linux prerelease
- Branch: `feature/tasia-giphy-welcome-loading-linux`
- Build run: `26061745761`
- Result: success
- Commit: `d73371e429172ae53b943a81a10426c73949bafd`
- Release: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-50`
- Asset: `Phoenix-FirestormOSTasia-Releasex64_LEGACY-8-0-1-78266.tar.xz`
- Release type: GitHub prerelease
- Next: runtime test Linux package before porting to Windows.

### 2026-05-18: Focused follow-up fixes after prerelease
- Scope limited to:
  - IM GIF button/support
  - pre-login `<USERNAME>` replacement
  - scheme-less YouTube URL detection in chat/IM previews
- Files changed:
  - `indra/newview/fsfloaterim.cpp`
  - `indra/newview/fsfloaterim.h`
  - `indra/newview/skins/default/xui/en/floater_fs_im_session.xml`
  - `indra/newview/fspanellogin.cpp`
  - `indra/newview/fspanellogin.h`
  - `indra/newview/llprogressview.cpp`
  - `indra/newview/llprogressview.h`
- Focused checks passed:
  - `git diff --check`
  - XML parse for IM, nearby chat, GIPHY picker, and settings XUI/XML
- Commit: `670d58e7ce Fix IM GIPHY and welcome username handling`
- GitHub Actions Linux build with `publish_release=true`: `26067438313`
- Result: success
- Release: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-51`
- Asset: `Phoenix-FirestormOSTasia-Releasex64_LEGACY-8-0-1-78266.tar.xz`

### 2026-05-19: Mom runtime checkpoint before sleep
- Working:
  - `welcome.txt` welcome message works well.
  - GIPHY picker/window opens.
  - GIPHY search works.
  - GIF/image results display in picker window.
  - Selecting GIF works.
  - GIF button exists in nearby chat.
  - GIF button/support now exists in IM windows.
- Not working yet:
  - Chat/IM embedding display does not work; likely needs a different implementation approach.
- Next:
  - Check GitHub Actions run `26067438313` and any autopublished release.
  - Fix embed display using another approach, without expanding unrelated scope.

### 2026-05-19: GIPHY preview correction
- Mom corrected status:
  - GIPHY chat preview did not show the actual GIF/image; it only showed a card/link.
  - YouTube panel rendered but playback failed with YouTube player error 153.
- Follow-up implementation:
  - `TasiaGiphyPreview` now includes a direct GIF media URL.
  - `tasiaExtractGiphyPreviewFromURL()` derives `https://i.giphy.com/media/<id>/giphy.gif` from normal GIPHY page URLs.
  - `TasiaGiphyPreviewPanel` now uses `LLMediaCtrl` to render the direct GIF URL wrapped in a tiny local image page instead of only showing text.
  - Preview insertion no longer requires `!use_plain_text_chat_history`, so compact/old chat style can attempt previews.
- YouTube correction:
  - Do not inject/load YouTube iframe directly from viewer-local/internal HTML.
  - Added hosted wrapper source: `web/youtube-player/index.html`.
  - Uploaded wrapper to `https://apps.easierit.org/igrid/youtube-player/`.
  - Viewer opens `https://apps.easierit.org/igrid/youtube-player/?v=VIDEO_ID` in `LLFloaterWebContent` when user clicks `Play in Viewer`.
  - Wrapper uses official YouTube IFrame API and `/embed/VIDEO_ID` from real HTTPS origin.
  - No autoplay with sound; user clicks play.
  - YouTube branding/controls remain visible.
  - Errors 101/150/153 show fallback text and Open on YouTube.
- Focused checks passed:
  - `git diff --check`
  - XML parse for IM, nearby chat, GIPHY picker, and settings XML.
  - HTML parse smoke for `web/youtube-player/index.html`.
- Needs runtime validation after GitHub build.

### 2026-05-19: Windows feature branch port
- Branch: `feature/tasia-giphy-welcome-loading-windows`
- Base: `github/windows-build-test`
- Ported commits:
  - `70f5906d0c Add Tasia GIPHY welcome chat previews`
  - `a37e373645 Fix IM GIPHY and welcome username handling`
  - `9f2c5e5c87 Render GIPHY previews with direct GIF media`
  - `cad8a16097 Add hosted YouTube player wrapper support`
- Windows workflow update:
  - `actions/checkout` now uses `ref: ${{ github.ref_name }}`.
  - Configure step passes `TASIA_GIPHY_API_KEY` from GitHub Secrets.
- Focused checks passed:
  - `git diff --check`
  - `python3 -m py_compile scripts/generate_tasia_giphy_key.py`
  - XML parse for settings, nearby chat, IM, GIPHY picker, and progress panel.
  - HTML parse smoke for `web/youtube-player/index.html`.
  - Conflict-marker scan of memory files.
- Next: push branch and trigger Windows build.
- Build triggered:
  - Run: `26098421990`
  - URL: `https://github.com/martysl/tasia-viewer/actions/runs/26098421990`
  - Commit: `ab1dd99400a63adb46061b2597dba8252984140f`
  - Inputs: `clean_build=false`, `probe_only=false`
  - Result: success
- Release status:
  - Published: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-16-windows`
  - Asset: `Tasia-Viewer-Windows-FMOD.zip`
  - Asset size: 461141078 bytes
  - Release type: GitHub prerelease
  - Target commit: `ab1dd99400a63adb46061b2597dba8252984140f`

### 2026-05-19: Next-build scope notes
- Voice/microphone issue:
  - Mom reports Windows microphone detection triggers immediately after app start even when voice chat is disabled.
  - Test with `LL_BAD_FMODSTUDIO_DRIVER=1` still triggered microphone detection, so FMOD is likely not responsible.
  - Official viewer reportedly does not show this behavior with voice disabled.
  - Likely suspect: viewer voice/WebRTC/Vivox startup initializes/enumerates capture devices before honoring `EnableVoiceChat=false`.
  - Desired behavior: if `EnableVoiceChat=false`, do not start voice backend or enumerate/query capture devices.
- Voice-disabled microphone detection hotfix candidate:
  - Files changed:
    - `indra/newview/llvoiceclient.cpp`
    - `indra/newview/llvoicewebrtc.cpp`
    - `indra/newview/llvoicevivox.cpp`
  - Behavior changed:
    - device refresh is skipped while `EnableVoiceChat=false`;
    - capture device selection is skipped while `EnableVoiceChat=false`;
    - mic gain/audio config setup is skipped while `EnableVoiceChat=false`;
    - WebRTC startup delays device refresh until voice is enabled.
  - Focused checks passed:
    - `git diff --check`
    - memory conflict-marker scan.
  - Needs Windows runtime test: launch with voice disabled and verify Windows microphone recent activity/indicator does not trigger.
- Renderer/engine future work:
  - Mom wants user-selectable rendering paths in viewer:
    - current PBR renderer/engine;
    - old pre-PBR renderer/engine from an older viewer line.
  - Pre-PBR source reference: `https://gitlab.com/lostorm/lostorm/-/tree/lostorm-13?ref_type=heads`.
  - This is a large feature and should be isolated in its own branch after the voice-disabled microphone issue is fixed.
  - Do not mix dual-renderer port with small hotfixes.

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
