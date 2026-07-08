# Texture Scanner — Content Moderation for Uploads

**Date:** 2026-07-05
**Source:** Mom & Tasia brainstorm

## Core Idea

Scan textures uploaded to the grid for inappropriate/illegal content using AI/ML vision models.
Flag or reject uploads based on severity, with reporting to admin panel or external endpoints.

## What Could Be Detected

| Category | Detectability | Method |
|----------|--------------|--------|
| CSAM / illegal content | Legally complex | PhotoDNA / closed APIs (not recommended without legal review) |
| Adult / NSFW content | ✅ High | ML vision models (nsfwjs, Ollama llava, Sightengine API) |
| Gore / extreme violence | ✅ High | ML vision classifiers |
| Spam / text in textures | ✅ Medium | OCR + text classification |
| Copyrighted material | ❌ Low | Needs reference DB; impractical for general use |
| Malware embedded in JPEG2000 | ❌ Extremely low | Texture pixels are rendered, not executed |

## Pipeline

```
User uploads → Asset cap receives JPEG2000 data
                       ↓
              Decode JPEG2000 → PNG (OpenJPEG)
                       ↓
              Send to scan queue (Redis / DB / in-memory)
                       ↓
              ML vision model classifies image
                       ↓
              ┌──── Clean ────┐ Flagged ───┐
              │               │            │
           Store            Soft flag   Hard flag
           normally         (warn)      (reject)
                                   
              Admin panel notification + optional external webhook
```

## Scanner Backend Options

| Option | Pros | Cons |
|--------|------|------|
| **Local Ollama** (`llava`, `minicpm-v`, `moondream`) | Free, private, no API key | GPU recommended, slower |
| **nsfwjs** (Node.js) | Fast, lightweight | NSFW only, no multi-class |
| **Sightengine API** | Accurate, multi-class | Paid, external dependency |
| **Aws Rekognition** | Scales well | Paid, external dependency |

Recommended initial approach: **Ollama + `llava`** running locally for privacy.

## Integration Points

### Upload-time scanning (inline)
- Hook into `UploadBakedTexture`, `UploadTexture`, `NewFileAgentInventory` caps
- Scan before storing final asset
- If flagged → reject upload with error message, or store with flag

### Background scanning (existing assets)
- Crawl existing texture assets from asset service
- Scan in low-priority queue
- Replace flagged textures with "Content Blocked" placeholder
- Generate report for admin

### Viewer-aware scanning
- **Tasia viewer users** → skip or light scan (trusted)
- **Other viewer users** → full scan pipeline + upload fees

## Reporting

- Flagged textures stored for admin review
- Admin panel (`hg/admin_panel.php`) shows:
  - Texture UUID
  - Preview thumbnail
  - Scan result / confidence score
  - Uploader avatar name + UUID
  - Timestamp
- Optional webhook to external reporting endpoint

## Legal / Safety Notes

- Do **not** store or serve confirmed CSAM — delete immediately
- Consider legal requirements for your jurisdiction before deploying
- PhotoDNA is restricted to approved organizations (Microsoft)
- When in doubt, reject/flag for manual review rather than auto-reporting
- Keep logs of who uploaded what for audit trail

## Implementation Phases

### Phase 1 — Basic Scanner Module
- **Module:** `TasiaAddons.TextureScanner`
- Decode JPEG2000 to PNG on upload
- Send to local Ollama endpoint (configurable URL)
- Log results, flag assets in DB
- No auto-reject yet — just report

### Phase 2 — Admin Panel UI
- Tab in admin panel for flagged textures
- Preview + details + ability to delete/replace
- Filter by status (pending, reviewed, clean, confirmed)

### Phase 3 — Enforcement
- Auto-reject uploads for flagged content (configurable threshold)
- Integrate with viewer perks — non-Tasia users get stricter scanning
- Webhook for external reporting

### Phase 4 — Background Crawler
- Scan existing asset store
- Report findings
- Optional: replace with placeholder
