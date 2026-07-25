# Opus 5 Three Games

Three games built from scratch to benchmark Claude Opus 5, for a YouTube video.
Each targets **~10 minutes of real, replayable gameplay with an actual end state**.

| # | Codename | Homage to | Engine | Machine | Directory |
|---|----------|-----------|--------|---------|-----------|
| 1 | **STORMFALL** | Fortnite | Unreal Engine 5.8 | Mac Studio | `01-stormfall-ue/` |
| 2 | **STRIKE PROTOCOL** | CS:GO | Godot 4.7 (3D) | MacBook | `02-strike-godot/` |
| 3 | **TEACUP** | Cuphead | Godot 4.7 (2.5D) | MacBook | `03-teacup-godot/` |

All three are single-player against bots. All assets are original — no shipped-game assets, names, logos,
maps, or likenesses are used.

## Start here

- **[`START-HERE.md`](START-HERE.md)** — read this first in a new session: how to run
  and verify all three games, what is and isn't verified, and the traps in this setup.
- **[`PLAYING.md`](PLAYING.md)** — controls and launchers.
- **[`GOAL.md`](GOAL.md)** — the original build spec.
- **[`docs/ENVIRONMENT.md`](docs/ENVIRONMENT.md)** — verified machines, engine paths, toolchain versions.
- **[`docs/ART-PIPELINE.md`](docs/ART-PIPELINE.md)** — how assets are generated and regenerated.

## Running a game

```bash
# STORMFALL (on the Mac Studio)
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" \
  ~/dev/Opus-5-Three-Games/01-stormfall-ue/Stormfall.uproject

# STRIKE PROTOCOL / TEACUP
godot --path 02-strike-godot     # /Applications/Godot.app/Contents/MacOS/Godot on the Studio
godot --path 03-teacup-godot
```

## Assets are not in this repo

Binaries (`*.png`, `*.fbx`, `*.wav`, `*.uasset`, …) are gitignored. Each game's
`assets/manifest.json` records the generation prompt (or script + args), output path, and sha256 for
every asset. After a fresh clone:

```bash
tools/artgen/rebuild.sh <game-dir>      # regenerates missing images and audio
```

Everything is generated — no asset-store downloads:

| | Tool |
|---|---|
| Images | `tools/artgen/gen.sh` → Codex CLI (MacBook only) |
| Audio | `tools/audiogen/synth.py` → numpy synthesis (both machines, deterministic) |
| Meshes | `tools/blender/` → headless Blender (Studio) |
| STORMFALL characters | Unreal's bundled Manny/Quinn + 102 animations (UE projects only) |

See [`docs/ART-PIPELINE.md`](docs/ART-PIPELINE.md).

## Branches

`main` holds docs and tooling. Each game is developed on `game/stormfall`, `game/strike`, `game/teacup`.
