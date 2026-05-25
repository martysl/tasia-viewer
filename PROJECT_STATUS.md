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

## Exact last failing step
- None yet for this patch; no compile/build attempted after the edits.

## What must not be changed
- Existing GIPHY/welcome/chat preview behavior.
- Existing remote config JSON schema and fallback badge behavior.

## Next exact action
- Commit the badge image loading patch, push branch `feature/tasia-tag-badge-fix`, and run focused Linux/Windows CI builds.
