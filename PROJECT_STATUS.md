# Project Status

## Current Phase

Linux viewer builds and runs with FMOD.

Current branch: `feature/tasia-giphy-welcome-loading-windows`.

Current feature work: Linux GIPHY/welcome/chat preview work succeeded and was ported to a Windows feature branch for validation/release.

Next work:
1. **Grid lock**: remove/block Second Life and add I-Grid Beta.
2. **Version/build identity**: internal 8.0.1, display 8.0.1.<GitHub run number>, show short commit SHA.
3. **Snapshot system**: replace Flickr/Primfeed with real TasiaFeed upload.
4. **TasiaFeed backend**: PHP + DB + WebDAV storage under apps.easierit.org/igrid/feed/.
5. **Bug reporting**: adapt BugSplat/crash reporter into real TasiaBugReport/TasiaCrash.
6. **Bug backend**: PHP + DB/admin under apps.easierit.org/igrid/bugs/.
7. **Branding cleanup** while keeping legal credits intact.
8. **Manual Linux build** and test.

## 2026-05-18: GIPHY/welcome/loading branch status

### What is done
- Windows feature branch port is prepared:
  - Branch: `feature/tasia-giphy-welcome-loading-windows`
  - Base: `github/windows-build-test`
  - Cherry-picked approved feature commits from Linux branch.
  - Windows workflow now checks out `${{ github.ref_name }}` so dispatched feature-branch builds use the Windows feature branch.
  - Windows configure step passes `TASIA_GIPHY_API_KEY` from GitHub Secrets.
  - Focused checks passed on the Windows feature branch.
- Mom runtime report before sleep:
  - `welcome.txt` welcome message works well.
  - GIPHY picker/window opens.
  - GIPHY search works.
  - GIF/image results display in picker window.
  - Selecting GIF works.
  - Nearby chat GIF button works.
  - IM windows now also have the GIF button/support.
- Mom correction after runtime testing:
  - GIPHY chat preview did **not** show the actual GIF/image; it only showed a card/link.
  - YouTube preview panel appeared, but playback failed with YouTube player error 153.
- Current follow-up fix attempt:
  - GIPHY chat preview now derives `https://i.giphy.com/media/<id>/giphy.gif` from normal GIPHY page URLs and renders that direct GIF in the chat preview panel.
  - Preview insertion no longer depends on `PlainTextChatHistory`, so compact/old chat style can attempt previews too.
- Hosted YouTube player wrapper support added as a separate checkpoint:
  - Wrapper source: `web/youtube-player/index.html`
  - Hosted URL: `https://apps.easierit.org/igrid/youtube-player/?v=VIDEO_ID`
  - Viewer YouTube chat card no longer injects an iframe locally.
  - User clicks `Play in Viewer`, then the viewer opens the hosted wrapper in `LLFloaterWebContent`.
  - The hosted wrapper uses the official YouTube IFrame API with `/embed/VIDEO_ID`, real HTTPS origin, no autoplay with sound, visible controls/branding, and Open on YouTube fallback.
  - Wrapper was uploaded by FTP and verified over HTTPS.
- Latest Linux GitHub Actions build with autopublish succeeded:
  - Run: `26067438313`
  - Commit: `670d58e7ceb4ac0429cfd31e47d2e5897445df07`
  - Release: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-51`
  - Asset: `Phoenix-FirestormOSTasia-Releasex64_LEGACY-8-0-1-78266.tar.xz`
- Focused follow-up fixes added after Linux prerelease:
  - IM windows now have a `GIF` button wired to the existing shared `LLFloaterGiphyPicker`.
  - Picker selection from IM sends the selected GIPHY URL to that IM session, not nearby chat.
  - YouTube chat/IM preview detection now accepts common scheme-less links like `youtube.com/...`, `www.youtube.com/...`, and `youtu.be/...`.
  - `welcome.txt` `<USERNAME>` replacement now keeps the raw chosen line and renders using best available name.
  - Pre-login welcome name priority: typed login username, saved/remembered username, then `friend` fallback.
  - After login/teleport, real avatar name is preferred when available.
- Linux GitHub Actions build for the GIPHY/welcome/chat preview branch succeeded:
  - Run: `26061745761`
  - Commit: `d73371e429172ae53b943a81a10426c73949bafd`
- Linux prerelease published:
  - `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-50`
  - Asset: `Phoenix-FirestormOSTasia-Releasex64_LEGACY-8-0-1-78266.tar.xz`
- Added build-time generated obfuscated GIPHY API key fallback support.
- Added generated-file ignores for:
  - `indra/newview/lltasia_giphy_key.generated.h`
  - `indra/newview/lltasia_giphy_key.generated.cpp`
- Added runtime key accessor files:
  - `indra/newview/lltasia_giphy_key.h`
  - `indra/newview/lltasia_giphy_key.cpp`
- Added generator script:
  - `scripts/generate_tasia_giphy_key.py`
- Wired Linux workflow configure step to pass `TASIA_GIPHY_API_KEY` from GitHub Secrets without printing it.
- Added Tasia settings for welcome text, GIPHY, animated chat preview, and optional loading YouTube embed.
- Added async welcome text client:
  - `indra/newview/lltasia_welcome_client.h`
  - `indra/newview/lltasia_welcome_client.cpp`
- Hooked `LLProgressView` to request one random usable line during startup loading and ignore late responses after startup completes.
- Added standalone GIPHY API client:
  - `indra/newview/llgiphyclient.h`
  - `indra/newview/llgiphyclient.cpp`
- `LLGiphyClient` supports search/trending, safe key lookup, rating setting, JSON parsing, and `GIPHY is not configured.` fallback.
- Added registered GIPHY picker floater shell:
  - `indra/newview/llfloatergiphypicker.h`
  - `indra/newview/llfloatergiphypicker.cpp`
  - `indra/newview/skins/default/xui/en/floater_giphy_picker.xml`
- Registered floater as `giphy_picker` in `indra/newview/llviewerfloaterreg.cpp`.
- Added nearby chat `GIF` button in `indra/newview/skins/default/xui/en/floater_fs_nearby_chat.xml`.
- Wired nearby chat button to `LLFloaterGiphyPicker`; selected GIF sends the normal GIPHY page URL through existing nearby chat send path.
- Added local GIPHY URL preview cards in `indra/newview/fschathistory.cpp`, gated by `TasiaAnimatedGifChatPreview`.
- Added direct image URL previews in `indra/newview/fschathistory.cpp`, gated by `TasiaImageChatPreview`.
- Added inline YouTube embeds in `indra/newview/fschathistory.cpp`, gated by `TasiaYouTubeChatPreview` and enabled by default.
- Confirmed active nearby chat and Firestorm IM use `FSChatHistory`; legacy `LLChatHistory` path is currently disabled by `#if 0`.
- Added loading panel branding text and `Powered by GIPHY` credit in `indra/newview/skins/default/xui/en/panel_progress.xml`.
- Added optional loading YouTube embed behavior in `LLProgressView`, gated by `TasiaLoadingYouTubeEnabled` and `TasiaLoadingYouTubeURL`; loading media is disabled by default.

### What is broken
- Windows build/release for this feature branch has not been run yet.
- Latest GIPHY direct GIF preview fix is not built/runtime-tested yet.
- YouTube embed panel renders, but playback may still fail with YouTube error 153 if the viewer media browser cannot satisfy YouTube embed requirements.
- Hosted YouTube wrapper playback is not runtime-tested in the viewer yet.
- Linux build succeeded, but runtime testing of the released package is still needed.

### What was last attempted
- Ported current approved Linux feature commits to `feature/tasia-giphy-welcome-loading-windows`.
- Updated `.github/workflows/build-windows.yml` to build the dispatched ref and pass `TASIA_GIPHY_API_KEY`.
- Focused checks passed on Windows feature branch: whitespace, Python generator compile, XUI/settings XML parse, wrapper HTML smoke, and conflict-marker scan.
- Added only the requested focused fixes: IM GIF button/support and pre-login `<USERNAME>` replacement.
- Added small YouTube detection fix for scheme-less YouTube URLs.
- Ran focused XML/whitespace checks successfully.
- Committed focused fixes as `670d58e7ce Fix IM GIPHY and welcome username handling`.
- Pushed branch and triggered GitHub Actions Linux build with `publish_release=true`: run `26067438313`.
- Run `26067438313` completed successfully and autopublished prerelease `v8.0.1-51`.
- Mom reported runtime progress: welcome message works well; GIPHY button/support now exists in IMs; embedding display does not work yet.
- Mom corrected status: GIPHY did not preview the actual GIF/image. Implemented direct GIPHY GIF media preview attempt in `FSChatHistory`.
- Focused checks passed after the direct GIF preview change.
- Added hosted YouTube player wrapper support and uploaded `web/youtube-player/index.html` to `https://apps.easierit.org/igrid/youtube-player/`.
- Changed YouTube chat card to open the hosted player wrapper in a web-content floater via `Play in Viewer`.
- Focused XML/HTML/whitespace checks passed.
- Generated empty local fallback key files with no secret present.
- Tested the generator with a fake key under `/tmp/opencode` and verified plaintext is not written.
- Parsed `indra/newview/app_settings/settings.xml` successfully.
- Ran `git diff --check` successfully.
- Added welcome client/progress hook and reran focused XML/whitespace checks successfully.
- Added `LLGiphyClient` and reran focused XML/whitespace/script checks successfully.
- Added `LLFloaterGiphyPicker`, registered it, added XUI, and reran focused XML/whitespace/script checks successfully.
- Wired nearby chat GIPHY button/selection send path and reran focused XML/whitespace/script checks successfully.
- Added GIPHY chat preview cards, loading branding/GIPHY credit, optional YouTube loading media, and reran focused XML/whitespace/script checks successfully.
- Enabled loading YouTube by default, added image URL chat previews, verified active IM coverage through `FSChatHistory`, and reran focused XML/whitespace/script checks successfully.
- Corrected YouTube behavior: chat/IM YouTube embeds are now enabled by default via `TasiaYouTubeChatPreview`; loading YouTube is optional/off by default. Focused checks passed again.
- Published Linux prerelease `v8.0.1-50` from successful run `26061745761`.

### Exact last failing step
- None in this session.

### What must not be changed
- Do not hardcode, print, commit, or document the real GIPHY API key.
- Do not commit generated GIPHY key files.
- Do not disturb the known-good Windows build path while Linux feature work is incomplete.
- Keep Linux feature work on `feature/tasia-giphy-welcome-loading-linux` until Linux build succeeds.

### Next exact action
- Commit/push Windows workflow ref fix and memory update, trigger Windows GitHub Actions build from `feature/tasia-giphy-welcome-loading-windows`, then publish a Windows prerelease from the artifact if the build succeeds.

## Build Status

| Platform | Build | Runtime | TasiaFeed upload |
|----------|-------|---------|------------------|
| Linux    | ✅ v0.1.0 | ✅ (basic login) | ❌ HTTP bug (postJsonAndSuspend fix applied, new build needed) |
| Windows  | ⏳ blocked on Linux first | - | - |
| macOS    | ⏳ blocked on Windows first | - | - |

## Completed Milestones

- v0.1.0 release (tagged) - Linux builds and runs with FMOD
- FMOD integration working (private deps pipeline)
- KDU removed
- Second Life grids removed from bundled defaults
- release branch created
- GitHub Actions: manual-only workflow, single-platform
- GCC -Wmaybe-uninitialized fixed (real fix, not suppression)
- **Grid Lock added**: I-Grid Beta included, SL grids blocked programmatically at all entry points, startup purge of existing SL grids
- **Version bumped to 8.0.1**: Display version 8.0.1.<GitHub run number>, commit SHA visible in About window
