# Environment

Verified 2026-07-24. Re-verify before assuming any of this still holds.

## Machines

### Mac Studio — `midir@192.168.1.157` (`midirstudio.local`)
Builds **all three games**. Apple M3 Ultra, macOS 26.5.2.

| Thing | Value |
|---|---|
| Unreal Engine | **5.8** — `/Users/Shared/Epic Games/UE_5.8` (47 GB) |
| UE build script | `/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh` |
| UE editor | `/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor` |
| Godot | `/Applications/Godot.app/Contents/MacOS/Godot` — **not on `PATH`** |
| Blender | `/Applications/Blender.app` (headless: `Blender.app/Contents/MacOS/Blender -b -P script.py`) |
| Xcode | 26.6 |
| git | 2.50.1 — **`git-lfs` is NOT installed** |
| Codex CLI | **not installed** — art generation happens on the MacBook |
| Repo | `/Users/midir/dev/Opus-5-Three-Games` |

> ⚠️ **Disk: ~122 GB free of 1.8 TB.** A UE project plus its DerivedDataCache will claim 30–60 GB.
> Cap the DDC, prune `Saved/` between iterations, and raise a flag if free space drops below 30 GB.

### MacBook — `darkeatermidir@` local
**Codex image generation only** — everything else runs on the Studio. Apple M5, 32 GB RAM, ~268 GB free.

| Thing | Value |
|---|---|
| Godot | 4.7.1 stable — `/opt/homebrew/bin/godot` |
| Xcode | 26.6 |
| Codex CLI | 0.131.0 — `~/.npm-global/bin/codex` |
| Python | 3.14.5 |

## Codex image generation

Authenticated through the ChatGPT account in `~/.codex/auth.json` (`OPENAI_API_KEY` is null —
**no API key needed**). Generated images land in `~/.codex/generated_images/<session-uuid>/*.png`;
114 were already on disk from previous sessions, so the mechanism is real.

> ✅ **Verified working end-to-end on 2026-07-24.** `tools/artgen/gen.sh` produced a clean 1024²
> tileable concrete texture and recorded it in the manifest.
>
> This required upgrading Codex from 0.131.0 → **0.145.0** (`npm install -g @openai/codex@latest`);
> the config pins `model = "gpt-5.6-sol"`, which the older CLI rejected.

**Codex is for image generation only.** It is not used for meshes, animation, audio, or code in this
project — see `docs/ART-PIPELINE.md` for what handles those.

> Harmless noise in Codex runs: the Linear and Notion MCP servers in its config have expired OAuth
> tokens and log errors on startup. Ignore them.

## Unreal bundled content (STORMFALL)

UE 5.8 ships a complete third-person shooter animation set under
`Templates/TemplateResources/`, licensed by the UE EULA for use in Unreal projects. **This fully
resolves STORMFALL's humanoid animation problem — no downloads, no Mixamo.**

| Asset | Path (under `Templates/TemplateResources/`) |
|---|---|
| Skeletal meshes | `High/Characters/Content/Mannequins/Meshes/SKM_Manny_Simple`, `SKM_Quinn_Simple` |
| **102 animation sequences** | `High/Characters/Content/Mannequins/Anims/` |
| Control rigs | `.../Mannequins/Rigs/` — incl. `CR_Mannequin_FootIK`, `CR_Mannequin_Procedural` |
| Weapon meshes | `Standard/Weapons/Content/{Rifle,Pistol,GrenadeLauncher}/Meshes/SKM_*` |
| Shooter anim BPs | `Standard/Variant_Shooter/Content/Anims/ABP_TP_Rifle`, `ABP_FP_Weapon`, … |

The 102 sequences cover, for both Rifle and Pistol: 8-direction walk and jog, aim offsets, ADS idle,
fire, reload, equip, dry-fire, jump start / fall loop / recovery, directional hit reactions
(light / medium / heavy), and 6 directional death animations.

> ⚠️ **EULA scope:** this content is licensed for **Unreal Engine projects only**. It may not be
> exported into the Godot games. STRIKE PROTOCOL and TEACUP author their own — see `ART-PIPELINE.md`.

## Audio tooling

| Tool | MacBook | Studio |
|---|---|---|
| numpy | 2.4.6 | 2.0.2 |
| scipy | 1.18.0 | — |
| ffmpeg | ✅ | ❌ |
| sox / fluidsynth | ❌ | ❌ |

Sufficient. Audio is synthesized directly with numpy/scipy and encoded with ffmpeg on the MacBook.

Codex is **MacBook-only**. Generate here, then rsync to the Studio:

```bash
rsync -av --progress <local-assets>/ midir@192.168.1.157:~/dev/Opus-5-Three-Games/<game>/assets/
```

## Toolchain smoke test

Run this before writing any game code:

```bash
# Studio
ssh midir@192.168.1.157 '"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" -help | head -5; \
  /Applications/Godot.app/Contents/MacOS/Godot --version; df -h / | tail -1'

# MacBook
godot --version && codex --version
```

## Known gaps

- **No `git-lfs` on the Studio.** Intentional — binaries are gitignored, not versioned.
- **`godot` is not on the Studio's `PATH`.** Use the full `.app` binary path there.
- **Studio Python is 3.9.6** (MacBook is 3.14.5). Tooling must stay 3.9-compatible; `tools/artgen`
  is verified on both.
- **No `ffmpeg` on the Studio.** Encode audio on the MacBook and rsync.
