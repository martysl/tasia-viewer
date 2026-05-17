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
| 2026-05-17 | `linux` branch = Linux changes only. `windows-build-test` = Windows changes only. | Never push Windows-specific changes to `linux` — too risky, breaks Linux builds |
