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
- **llimview.cpp**: All 6 voice API mismatches fixed:
  1. LLVoiceChannelP2P constructor → added 4th arg (outgoingInterface)
  2. LLVoiceChannelGroup constructor → added 3rd arg (is_p2p = false)
  3. endUserIMSession → removed (no equivalent in new API)
  4. isValidChannel → replaced with getIncomingCallInterface check
  5. Two declineInvite calls → replaced with call->declineInvite() via incoming interface
  6. setSessionHandle → replaced with LLSD-based addP2PSession (matches header declaration)
- File restored to lostorm-13 baseline with voice API fixes applied (reverts PBR replacement)

## What is broken
- **Windows CI** still produces 194-byte placeholder artifact instead of real installer
- **Linux CI** currently in progress (run 26565850569, started 09:14Z May 28)
- Previous Linux run (26546679374) failed on llimview.cpp voice API mismatches; this run should get past that

## What was last attempted
- Fixed all 6 voice API calls in llimview.cpp
- Pushed to `martysl/tasia-viewer` repo, `tasia-legacy-port` branch
- Windows CI completed with "success" but produces 194-byte placeholder artifact
- Linux CI currently in progress

## What must not be changed
- llappviewer.cpp must remain lostorm-13 baseline with only our 2 includes
- Don't drag in PBR-era dependencies
- Preserve MsQuic/QUIC by deliberate port, not removal
- llimview.cpp must use lostorm-13 baseline with surgical voice API fixes, not PBR replacement

## Next actions
1. Wait for Linux CI to finish
2. If Linux CI passes (real artifact > 50 MiB), fix Windows CI
3. Runtime-test: QUIC padlock, emoji picker, voice
4. Clean up stale todo entries
