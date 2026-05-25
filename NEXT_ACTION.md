# NEXT ACTION

## Now: remote Tasia user config

1. Review local branch/worktree `feature/tasia-remote-user-config` at `/tmp/opencode/tasia-user-config`.
2. Create/verify server file: `https://i.let-us.cyou/hg/config.json` (currently HTTP 404).
3. If approved, commit/push and trigger builds.

Example supported JSON fields per user:

```json
{
  "uuid": "uuid_here",
  "custom_title": ":heart: Mom :heart:",
  "badge_name": "Developer",
  "badge_icon": "https://placehold.co/200x50?text=Developer&font=Poppins",
  "profile_text": "EasierIT Developer",
  "tooltip": "Official EasierIT Staff",
  "tag_color": "#ff66cc"
}
```

## Now
- Wait for builds #26180447827 (Linux) and #26180456294 (Windows) to complete
- Verify the voice mic detection fix works

## After builds pass
1. Test GIPHY picker works
2. Test welcome message appears
3. Test YouTube previews in chat
4. Test image previews in chat
5. **Test voice-disabled mic detection** — verify no microphone indicator when voice is disabled

## Now: profile badge image loading
1. Commit local patch in `/mnt/a/2026/tasia-tag-badge-fix`.
2. Push branch `feature/tasia-tag-badge-fix`.
3. Run Linux/Windows CI builds.
4. Runtime-test `badge_icon` with PNG/JPG HTTPS URLs, redirecting URLs, invalid URL fallback, and profile reset/stale response handling.
