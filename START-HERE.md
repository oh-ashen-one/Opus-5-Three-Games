# START HERE

Read this first if you are a new session picking this project up cold.

Three games built from scratch to benchmark Claude Opus 5, for a YouTube video.
All three are **finished and playable**. What remains is playtesting — feel,
framerate, and tuning — which needs a human at the keyboard.

---

## The one thing that matters about this project's setup

**Everything runs on the Mac Studio, not the MacBook.**

- Studio: `midir@192.168.1.157` — has UE 5.8, Godot 4.7, Blender, and the builds
- Repo on the Studio: `~/dev/Opus-5-Three-Games` (branch `main`)
- The MacBook has the source but **cannot run any of it**

SSH in and work in place. The one exception is Codex CLI image generation, which
is only authenticated on the MacBook.

⚠️ **The Desktop is iCloud-synced between both machines.** Deleting something from
the MacBook Desktop deletes it from the Studio too. This has already bitten once.

⚠️ **`rsync -a` preserves mtimes**, which can be older than the artefacts already
on the Studio, so UnrealBuildTool skips the rebuild and you test a stale binary.
Use `tools/sync-to-studio.sh <game-dir>`, which touches sources after copying.

---

## The three games

| # | Name | Homage | Engine | Directory |
|---|---|---|---|---|
| 1 | **STORMFALL** | Fortnite | Unreal Engine 5.8 | `01-stormfall-ue/` |
| 2 | **STRIKE PROTOCOL** | CS:GO | Godot 4.7 (3D) | `02-strike-godot/` |
| 3 | **TEACUP** | Cuphead | Godot 4.7 (2.5D) | `03-teacup-godot/` |

All single-player vs bots. All assets original — nothing from any shipped game.

---

## Playing them (the short version)

On the Mac Studio, open the Desktop folder **`Opus 5 Three Games`** and
double-click one of:

```
Play STORMFALL (packaged).command    <- standalone build, no editor
Play STORMFALL (editor).command      <- opens Unreal, then press Play
Play STRIKE PROTOCOL.command
Play TEACUP.command
```

Full controls are in [`PLAYING.md`](PLAYING.md). The essentials:

- **STORMFALL** — WASD, LMB fires *and harvests* (shoot trees for materials),
  **F** builds, Q cycles piece, Esc pauses
- **STRIKE** — WASD, LMB fires, B opens the buy menu in freeze time then 1-8 to
  purchase, 4/5/6 throw HE/smoke/flash. **Counter-strafe**: tap the opposite key
  to stop dead before shooting; firing while moving is heavily penalised
- **TEACUP** — Z dashes (i-frames), **C parries but only pink projectiles**,
  B is EX, V is super. Opens with a run-and-gun stage, then three bosses

⚠️ STORMFALL's packaged `.app` must stay beside its sibling `Engine/` and
`Stormfall/` folders. Moving the `.app` alone breaks it — that was the original
packaging bug.

---

## Verifying anything (all of this is reproducible)

```bash
# STORMFALL — 13 unit tests
cd ~/dev/Opus-5-Three-Games/01-stormfall-ue && ./Tools/run_tests.sh

# STORMFALL — 27 in-world integration checks
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "$PWD/Stormfall.uproject" -run=pythonscript \
  -script="$PWD/Tools/test_build_integration.py" -unattended -nosplash
cat /tmp/sf_integration_out.txt

# STRIKE — 148 assertions, then a full simulated match
GODOT=/Applications/Godot.app/Contents/MacOS/Godot
$GODOT --headless --path 02-strike-godot --script res://tests/run_tests.gd
$GODOT --headless --path 02-strike-godot res://scenes/sim.tscn

# TEACUP — 87 assertions, boss rush, and the intro stage
$GODOT --headless --path 03-teacup-godot --script res://tests/run_tests.gd
$GODOT --headless --path 03-teacup-godot res://scenes/sim.tscn
$GODOT --headless --path 03-teacup-godot res://scenes/sim_intro.tscn

# Render real frames and measure FPS (Godot games)
$GODOT --path 02-strike-godot res://scenes/capture.tscn
# PNGs land in ~/Library/Application Support/Godot/app_userdata/<NAME>/
```

Rebuild STORMFALL's standalone build: `01-stormfall-ue/Tools/build_game.sh`.
Regenerate its maps (deterministic): `Tools/gen_island.py`, `gen_greybox.py`,
`gen_mainmenu.py` via `UnrealEditor-Cmd -run=pythonscript`.

---

## Where things stand

**Verified, with reproducible output:** 248 test assertions, 27 in-world checks,
three headless game simulations. STRIKE plays full matches with real attrition
and bomb plants; TEACUP beats all three bosses and clears its intro stage;
STORMFALL's building, storm, loot, harvesting and combat all work in-world.
Measured framerate: STRIKE 117 fps mean, TEACUP 119 fps mean (M3 Ultra, 720p).

**Not verified — this is tomorrow's job:**

- Whether any of it *feels* good. No test answers this.
- STORMFALL's framerate with 15 bots and 518 actors.
- Whether the bots read as opponents or as furniture.
- A full STORMFALL match played through to Victory Royale.

**Known gaps, deliberately left:**

- STRIKE: bomb plants happen but stay uncommon; most rounds end by elimination.
  Ts do reach the sites (closest-approach went 1764 → 85 units against a 500-unit
  plant radius) — they just lose the entry fight more often than not.
- TEACUP: bosses have distinct silhouettes but are still built from primitives.
- No audio in any of the three. `tools/audiogen/synth.py` exists and works
  (numpy-only, deterministic) but nothing is wired up.

---

## Working notes for a new session

- Everything that decides an outcome is a **pure function**, tested headlessly.
  Actors and scenes are a thin layer over that. Keep it that way.
- The Godot test harnesses **fail if too few assertions run** — a parse error
  makes every static call silently no-op and would otherwise report a green run
  that asserted nothing. That guard has already caught one real failure.
- Bots share the player's classes and components in both shooters, so a bot
  cannot have stats the player doesn't.
- Binaries are gitignored. Maps, packaged builds and generated assets live only
  on the Studio and are regenerable from scripts.
- The headless simulations exist because unit tests could not catch the bugs
  that actually mattered: bots frozen in place by friction, bots with unlimited
  sight range, a player spawning permanently dashing, a HUD rendered off-screen.
  **When something seems fine but plays wrong, write a sim, not another unit
  test.**
