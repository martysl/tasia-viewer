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
