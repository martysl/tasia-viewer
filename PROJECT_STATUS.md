# Tasia Legacy Port — Project Status

## What is done
- QUIC backend fully ported to legacy (llcircuit, message, llstartup, llworld, llviewermessage, llviewergenericmessage, llstatusbar, notifications)
- bool→BOOL override fixes in 10 files (fsfloaternearbychat, llpanelprofile, llfloatergiphypicker, llthumbnailctrl, fschathistory, fsfloaterim, llprogressview)
- FMOD 2.03.07 API fix (F_CALLBACK → F_CALL)
- llappviewer.cpp restored from pristine lostorm-13 + 2 Tasia includes
- Voice API mismatches in llappviewer.cpp fixed (serverType→voiceServerType, leaveChannel→leaveNonSpatialChannel)
- Emoji picker fully ported and fixed for legacy APIs (llfloateremojipicker.cpp, llpanelemojicomplete.cpp, llemojidictionary, llemojihelper, emoji_groups.xml, XUI)
- llscreenchannel.h: added notification channel UUID constants
- loversion.h copied from current/PBR to legacy
- Legacy fsfloaterim.cpp/fsfloaternearbychat.cpp fixed for legacy font APIs
- **llimview.cpp**: All 6 voice API mismatches fixed, restored to lostorm-13 baseline
- **llvoavatar.cpp**: Restored from lostorm-13, Tasia tag badges re-applied
- **boost::json → nlohmann::json**: Replaced boost 1.83 dependency with nlohmann-json3-dev (CI, works alongside autobuild boost 1.72)
- **llvoicewebrtc.cpp/.h**: Added `workqueue.h` include, replaced PBR-only `LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE` with `_VOLUME`
- **llvoicewebrtc.h**: Fixed `LL::WorkQueue::weak_t` missing include
- **llvoicewebrtc.cpp**: Fixed `LLVector3d::operator[]` → `.mdV[]`, `LLQuaternion::operator[]` → `.mQ[]` (PBR APIs not in legacy)
- **postToMainCoro** (PBR-only): Replaced with direct `LLFirstUse::speak()` calls in `llvoicevivox.cpp` and `llvoicewebrtc.cpp`
- **CMakeLists.txt**: Added `llvoicewebrtc.cpp/.h` to the build
- **Linux CI passes**: 1375/1375 → links → packages → uploads real ~316 MB artifact

## CI branches
- `tasia-legacy-port` — **Linux CI only** (passes, produces real artifact)
- `tasia-legacy-port-win` — **Windows CI** (created to keep Windows fixes separate)

## What is broken
- **Windows CI** still produces 194-byte placeholder artifact instead of real installer
- Windows CI reports "success" but the artifact is empty/fake

## What was last attempted (Linux)
- CI run 26694360564 completed successfully on 2026-05-30
- Final fix: `LLVector3d::operator[]` / `LLQuaternion::operator[]` → direct member access
- All 1375 targets compiled without errors, linked, and packaged

## What must not be changed
- llappviewer.cpp must remain lostorm-13 baseline with only our 2 includes
- Don't drag in PBR-era dependencies
- Preserve MsQuic/QUIC by deliberate port, not removal
- llimview.cpp must use lostorm-13 baseline with surgical voice API fixes, not PBR replacement
- `tasia-legacy-port` branch stays Linux-only; Windows CI fixes go to `tasia-legacy-port-win`

## Next actions
1. Fix Windows CI on `tasia-legacy-port-win` branch
2. Runtime-test: QUIC padlock, emoji picker, voice (after Windows also passes)
3. Clean up stale workflow runs
