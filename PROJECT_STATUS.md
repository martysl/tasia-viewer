# PROJECT STATUS

## 2026-05-24 Remote Tasia user config feature

## What is done
- Created isolated local branch/worktree: `feature/tasia-remote-user-config` at `/tmp/opencode/tasia-user-config`.
- Added non-blocking one-shot startup fetch for `https://i.let-us.cyou/hg/config.json`.
- Added UUID-keyed custom user data for nametag title/color and profile badge text/tooltip.
- Changed welcome default URL to `https://i.let-us.cyou/welcome.php` and made welcome fetch use the first usable line instead of randomizing lines.
- Welcome fetch now resets per visible progress screen, so it can run for login and teleport loading screens.

## What is broken
- `https://i.let-us.cyou/hg/config.json` currently returns HTTP 404, so viewer-side code will fail gracefully but cannot display badges until the server file exists.
- Full viewer build not run yet.

## What was last attempted
- Focused local implementation and verification on `feature/tasia-remote-user-config`.

## Exact last failing step
- Remote config URL smoke test returned HTTP 404.

## What must not be changed
- Do not touch Mom's existing dirty main worktree changes.
- Do not push/build until Mom approves.

## Next exact action
- If Mom approves: commit, push `feature/tasia-remote-user-config`, and trigger Linux/Windows builds.

## What is done
- GIPHY key obfuscation, welcome text client, GIPHY picker, YouTube/image previews — all working in prior clean builds
- Previous clean builds (26135635543 Linux, 26135636272 Windows) completed successfully
- **Cherry-picked LL commit be04175579** (#4358 "Fix Microphone in use task bar icon") on both branches:
  - Gating updateSettings() with voiceEnabled check
  - Setting init_recording_on_send=false at WebRTC level
  - Proper SetAudioRecording() management
  - Mute state machine fix (MUTE_INITIAL/MUTED/UNMUTED)
- Pushed to both branches on GitHub

## What is broken
- Voice-disabled microphone detection — the fix is now applied, waiting for build results

## What was last attempted
- Cherry-picked LL commit be04175579 on both Linux and Windows branches
- Committed as eb97718a51 (Linux) / d9f04830f8 (Windows)
- Dispatched new clean builds with the fix:
  - Linux: #26180447827
  - Windows: #26180456294

## Exact last failing step
- Previous voice gating attempts (LLCoros cleanup, deferred init) did not work
- LL commit addresses root cause at WebRTC level (InitMicrophone/InitRecording) via init_recording_on_send=false

## What must not be changed
- Build-time obfuscated GIPHY key mechanism
- Generated source file handling (.generated.h/.cpp ignored)
- Welcome/GIPHY/YouTube/image chat preview features
- AGENTS.md, PROJECT_STATUS.md, NEXT_ACTION.md, BUILD_NOTES.md, DECISIONS.md, SECRETS_POLICY.md

## Next exact action
- Wait for builds #26180447827 and #26180456294 to complete
- Verify GIPHY picker, welcome text, YouTube/image previews still work
- Test voice-disabled mic detection fix on Windows

## 2026-05-25 Profile remote badge image loading

## What is done
- Moved Linux badge-fix worktree from `/tmp/opencode/tasia-fix` to persistent path `/mnt/a/2026/tasia-tag-badge-fix`.
- Reworked remote profile badge icons to fetch HTTP image bytes directly and decode via `LLViewerTextureList::getImageFromMemory(...)`.
- Added `LLThumbnailCtrl::setTexture(...)` for displaying decoded in-memory badge textures.
- Added redirect following, 60-second timeout, no retries, and a 10 MiB/range-limited badge fetch guard.
- Kept fallback behavior to `Profile_Badge_Team` if badge fetch or decode fails.

## What is broken
- Full viewer build has not been run for this badge image loading patch yet.

## What was last attempted
- Focused code review/static sanity pass on profile badge image loading patch.
- CI builds were triggered after commit `4ed7ac0c21`.
- Removed profile badge URL from hover tooltip; hover now uses configured tooltip/profile text only.

## Exact last failing step
- Linux run `26417570550` and Windows run `26417571261` failed compiling `indra/newview/llpanelprofile.cpp` because `LLViewerTextureList::getImageFromMemory(...)` is private.

## What must not be changed
- Existing GIPHY/welcome/chat preview behavior.
- Existing remote config JSON schema and fallback badge behavior.

## Next exact action
- Commit and push fix to use public `LLViewerTextureManager::getFetchedTextureFromMemory(...)`, then rerun focused Linux/Windows CI builds.

## 2026-05-26 Badge release

## What is done
- Latest badge fallback builds passed:
  - Linux run `26422421054` / commit `fbec75918c`
  - Windows run `26422421559` / commit `bef3638071`
- Published prereleases:
  - Linux `v8.0.1-20`: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-20`
  - Windows `v8.0.1-47-windows`: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-47-windows`
- Deleted older visible releases `v8.0.1-17` and `v8.0.1-44-windows`.
- Deleted older current-viewer Actions runs, keeping latest Linux/Windows release runs visible.

## What is broken
- Nothing release-blocking currently known.

## What was last attempted
- Posted formatted Discord message with separator lines using Mom-provided webhook.

## Exact last failing step
- Earlier local webhook from `~/.config/opencode/token.txt` returned HTTP 403; Mom-provided webhook succeeded with HTTP 204 when sent with a User-Agent header.

## What must not be changed
- Published release tags/assets unless replacing with a new build.

## Next exact action
- Runtime-test the new release builds, especially profile badge fallback/loading behavior.

## 2026-05-26 Built-in profile badge names

## What is done
- Added support for using built-in profile badge texture names directly in `badge_name`.
- Supported names: `Profile_Badge_Beta`, `Profile_Badge_Beta_Lifetime`, `Profile_Badge_Lifetime`, `Profile_Badge_Linden`, `Profile_Badge_Pplus_Lifetime`, `Profile_Badge_Premium_Lifetime`, `Profile_Badge_Team`.
- Remote `badge_icon` URL still takes priority; built-in `badge_name` is used as fallback while remote image loads or if no URL is provided.

## What is broken
- Not built yet after the built-in badge-name change.

## What was last attempted
- Static patch and whitespace checks.

## Exact last failing step
- None yet for this change.

## What must not be changed
- Existing URL-based remote badge behavior and fallback team badge behavior.

## Next exact action
- Commit/push Linux and Windows badge branches and run CI.
