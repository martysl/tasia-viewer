# Viewer Perks / Loyalty System

**Date:** 2026-07-05
**Source:** Mom & Tasia brainstorm

## Core Idea

Users connecting with Tasia Viewer (channel `Tasia-Releasex64`) get premium perks.
Users connecting with other viewers (Firestorm, Singularity, Cool VL, etc.) pay standard/full rates.

## Why This Works

Every login to Robust already logs the viewer channel:

```
LOGIN SERVICE: Login request for <name> at last using viewer 8.0.1.78498, channel Tasia-Releasex64, IP ...
```

The `channel` field is available server-side during login and can be stored/looked up per user.

## Proposed Perks for Tasia Viewer Users

| Perk | Description |
|------|------------|
| **God mode** | God-level permissions on regions |
| **Weekly stipend** | Auto-credit money to wallet |
| **Free upload** | Textures, meshes, sounds — no charge |
| **Sim management** | Region restart, OAR, estate controls via self-service |
| **Self-service panel** | Web/In-world panel for account management |
| **AI features** | Access to LLM/text generation from in-world scripts |

## Pricing for Non-Tasia Viewer Users

- **50 L$ per upload item** (texture, mesh, sound, animation)
- **Mesh: proportional to resource usage** (vertex count, triangle count, LOD complexity)
- Standard market/economy rates for everything else

## Technical Approach

### Option A: Check at Upload Caps
Hook into the asset upload caps (`UploadBakedTexture`, `UploadMesh`, `NewFileAgentInventory`, etc.) and:
1. Look up the user's last viewer channel
2. If Tasia → skip fee, allow
3. If other → apply pricing, then allow

### Option B: Check at Login, Store Permission
1. During login, note the viewer channel
2. Store a `tasia_viewer_user = 1` flag in the user's account data
3. Use this flag throughout the session for all gated features

### Option C: Hybrid
- Tasia viewer users get recorded at login
- Their session/agent gets an internal god-mode override
- Upload caps, stipend logic, and other perks check this override

## Implementation Plan

### Phase 1 — Detection Module
- **Module:** `TasiaAddons.ViewerPerks`
- On login: read viewer channel, log it, store in memory/session
- Add `/admin/perks` console command to list/view perks status

### Phase 2 — Upload Pricing
- Modify upload caps to check viewer channel
- Free for Tasia, 50 L$ + mesh cost for others
- Need to calculate mesh resource usage (vertices, triangles, LOD)

### Phase 3 — Perks Delivery
- Weekly stipend (money server integration)
- God mode flag
- Self-service panel access
- AI feature gating

### Phase 4 — Admin Panel
- Web UI in `hg/admin_panel.php` to:
  - See which users are Tasia viewers
  - Manually grant/revoke perks
  - View upload fee logs
