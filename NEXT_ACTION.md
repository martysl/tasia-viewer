# Next Action

## Current
1. ✅ Created Linux feature branch: `feature/tasia-giphy-welcome-loading-linux`.
2. ✅ Added generated obfuscated GIPHY key fallback infrastructure.
3. ✅ Added Tasia welcome/GIPHY/loading settings.
4. ✅ Implemented async welcome.txt random-line fetch and hooked it into `LLProgressView`.
5. ✅ Implemented `LLGiphyClient` search/trending support and no-key fallback.
6. ✅ Implemented and registered `LLFloaterGiphyPicker` with search/trending list UI and `Powered by GIPHY` footer.
7. ✅ Added nearby chat `GIF` button and wired picker selection to send the selected normal GIPHY page URL.
8. ✅ Added local GIPHY URL preview cards in chat history behind `TasiaAnimatedGifChatPreview`.
9. ✅ Added direct image URL previews in chat/IM history behind `TasiaImageChatPreview`.
10. ✅ Added YouTube embeds in chat/IM history behind `TasiaYouTubeChatPreview` enabled by default.
11. ✅ Added loading panel Tasia branding/GIPHY credit and optional loading YouTube media, disabled by default.
12. ✅ Verified active nearby chat and Firestorm IM use `FSChatHistory`; legacy `LLChatHistory` path is disabled by `#if 0`.
13. ✅ Ran focused generator/XML/whitespace verification.
14. ✅ Linux GitHub Actions build succeeded: `26061745761`.
15. ✅ Linux prerelease published: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-50`.
16. ✅ Added focused IM GIF button/support using the existing shared GIPHY picker.
17. ✅ Fixed pre-login `<USERNAME>` welcome replacement with typed/saved/friend fallback and raw-line re-rendering.
18. ✅ Fixed YouTube chat/IM preview detection for common scheme-less YouTube links.
19. **Review/commit the focused fixes, then trigger a new Linux GitHub Actions build with `publish_release=true`.** ← NOW

## Blockers
- None currently.
