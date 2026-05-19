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
19. ✅ Committed and pushed focused fixes: `670d58e7ce Fix IM GIPHY and welcome username handling`.
20. ✅ Triggered Linux GitHub Actions build with `publish_release=true`: `26067438313`.
21. ✅ Mom runtime report saved: welcome message works well; GIPHY picker/search/select works; nearby chat and IM GIF buttons exist; embedding display does not work yet.
22. ✅ Run `26067438313` succeeded and autopublished prerelease `v8.0.1-51`.
23. ✅ Mom corrected status: GIPHY preview did not show the actual GIF/image; it only showed a card/link.
24. ✅ Implemented direct GIPHY GIF media preview attempt using `https://i.giphy.com/media/<id>/giphy.gif`.
25. ✅ Kept compact/old chat style preview insertion enabled.
26. ✅ Added hosted YouTube player wrapper support: `https://apps.easierit.org/igrid/youtube-player/?v=VIDEO_ID`.
27. ✅ Uploaded wrapper page to the HTTPS domain and verified it loads.
28. ✅ Changed YouTube chat card to open the hosted wrapper in `LLFloaterWebContent` when user clicks `Play in Viewer`.
29. ✅ Ported approved feature commits to Windows feature branch `feature/tasia-giphy-welcome-loading-windows`.
30. ✅ Updated Windows workflow to checkout the dispatched ref and pass `TASIA_GIPHY_API_KEY` during configure.
31. ✅ Focused Windows-branch checks passed.
32. ✅ Pushed Windows feature branch and triggered Windows build `26098421990` with `clean_build=false`, `probe_only=false`.
33. ✅ Windows build `26098421990` succeeded.
34. ✅ Windows prerelease published: `https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1-16-windows`.
35. **Runtime-test Windows prerelease `v8.0.1-16-windows`.** ← NOW

## Blockers
- None currently.
