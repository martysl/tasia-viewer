# Next Action

## Current
1. Deploy/sync `server/tasiafeed/owner.php` plus updated `index.php`/`post.php` to `https://apps.easierit.org/igrid/feed/` if Mom wants it live now.
2. If continuing Linux upload UX, investigate true OS file drag/drop via lower-level Linux window/drop-event plumbing.

## Latest verified
- PHP syntax checks passed for TasiaFeed `owner.php`, `index.php`, and `post.php`.
- This TasiaFeed fix is PHP-only and does not require a viewer build.

<<<<<<< HEAD
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
=======
## Current
1. ✅ Latest releases published:
   - Linux `v8.0.1-17`
   - Windows `v8.0.1-44-windows`
2. ✅ Old releases and old Actions run results cleaned up.
3. ✅ Discord release announcement sent with direct ZIP links.
>>>>>>> 43da2163ac (Rebrand to Tasia: channel name, URLs, auto revision)

## Next
1. Runtime-test both releases with real `config.json` entries (2+ users, different `tag_color`, different badge icons).
2. Confirm nametag full-color behavior and profile badge visibility across grids.
3. If stable, keep this as baseline release procedure.

## Blockers
- None currently.
