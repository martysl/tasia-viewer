# Decisions

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-05-16 | FMOD required, not optional | Viewer needs working audio |
| 2026-05-16 | KDU removed permanently | No license available |
| 2026-05-16 | Manual single-platform builds only | Avoid wasted CI minutes, easier debugging |
| 2026-05-16 | Grid lock: block SL programmatically | Not just UI-hide, real guard at login |
| 2026-05-16 | No fake features | A feature is done only when Mom can use it |
| 2026-05-16 | Build order: Linux → Windows → macOS | Linux is most accessible for development |
| 2026-05-16 | FMOD packages stay private always | Licensing restriction |
| 2026-05-16 | Real fix over warning suppression | Fixed LLTweenableValueLerp constructors instead of using -Wno-error |
| 2026-05-16 | SL grid blocking: URI-driven, not name-driven | Avoids false positives from grids sharing names with SL servers |
| 2026-05-16 | Version 8.0.1 with GitHub run number as build metadata | Run number is traceable and unique |
| 2026-05-18 | GIPHY API key comes from `TASIA_GIPHY_API_KEY` at build time or `TasiaGiphyAPIKey` at runtime | Avoids committing real keys while still allowing a packaged fallback |
| 2026-05-18 | Generated GIPHY key source files are ignored build artifacts | Prevents accidental commits of generated key material |
| 2026-05-18 | Welcome text fetch must be best-effort and startup-only | Network failure or late responses must not replace valid server/grid progress text |
| 2026-05-18 | GIPHY request URLs must not be logged | GIPHY API key is carried in the request query string |
| 2026-05-18 | Chat GIPHY preview starts as a safe local card, not inline media playback | Avoids heavy media/browser instances per chat line while preserving normal URL compatibility |
| 2026-05-18 | YouTube embeds are chat/IM previews by default, not loading-screen media | Matches expected behavior when someone sends a YouTube URL |
| 2026-05-18 | Loading YouTube media stays optional/off by default | Prevents unexpected remote media during startup |
| 2026-05-18 | Direct image previews are implemented in active `FSChatHistory` only | Nearby chat and Firestorm IM use this path; legacy `LLChatHistory` is disabled by `#if 0` |
| 2026-05-19 | Fix voice-disabled microphone detection before dual-renderer work | Privacy issue is smaller and more urgent than renderer architecture |
| 2026-05-19 | Dual renderer/PBR vs pre-PBR must be a separate feature branch | Renderer architecture is high-risk and must not mix with hotfixes |
| 2026-05-25 | Keep only the newest Linux+Windows releases visible | Avoids release confusion and stale downloads |
| 2026-05-25 | Prune old Actions run results after successful publish | Keeps CI history focused on current releasable builds |
| 2026-05-25 | Discord webhook post is part of release completion | Ensures Mom/community always gets direct release+ZIP links |
| 2026-08-31 | KLIPY picker off by default (`TasiaKlipyEnabled=false`) | Disabled provider must not initialize; opt-in avoids key/network use until enabled |
| 2026-08-31 | KLIPY API key via `TASIA_KLIPY_API_KEY` at build time or `TasiaKlipyAPIKey` at runtime, same as GIPHY | Avoids committing real keys while allowing a packaged fallback |
| 2026-08-31 | Generated KLIPY key source files are ignored build artifacts | Prevents accidental commits of generated key material |
| 2026-08-31 | KLIPY request URLs must not be logged | KLIPY API key is carried in the request query string |
