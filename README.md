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

- **[`GOAL.md`](GOAL.md)** — the full build spec. Paste it into a fresh Claude Code session to begin.
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
`assets/manifest.json` records the generation prompt, output path, and sha256 for every asset.
After a fresh clone:

```bash
tools/artgen/rebuild.sh <game-dir>
```

This regenerates the art from the manifest. It requires Codex CLI, which lives on the MacBook.

## Branches

`main` holds docs and tooling. Each game is developed on `game/stormfall`, `game/strike`, `game/teacup`.
