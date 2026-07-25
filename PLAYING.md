# Playing all three

Everything runs on the Mac Studio (`midir@192.168.1.157`), repo at
`~/dev/Opus-5-Three-Games`.

**Quickest start:** double-click the files in `launchers/` —
`Play STORMFALL.command`, `Play STRIKE PROTOCOL.command`, `Play TEACUP.command`.

---

## 1. STORMFALL — Unreal Engine 5.8

```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" \
  ~/dev/Opus-5-Three-Games/01-stormfall-ue/Stormfall.uproject
```

Opens the editor on `Lvl_Island`. Press **Play → Standalone Game** for a clean view.

Launching the *game* (rather than the editor) now boots to `Lvl_MainMenu`; choose
**PLAY** to load the island. Arrows + Enter navigate menus.

| Input | Action |
|---|---|
| `WASD` / mouse | Move / look |
| `Shift` / `Ctrl` / `Space` | Sprint / crouch / jump |
| **LMB** | Fire — also harvests: shoot trees and rocks for materials |
| `R` | Reload |
| **`F`** | Place the selected build piece |
| `Q` | Cycle wall → ramp → floor → roof |
| `Esc` | Pause |

Maps: `Lvl_MainMenu` (boot), `Lvl_Island` (the match — 8 POIs, 15 bots, storm),
`Lvl_Greybox` (movement tuning, drop-test ledges).

**Packaged build (no editor needed):** `launchers/Play STORMFALL (packaged).command`,
or `01-stormfall-ue/Dist/Mac/Stormfall.app`.

⚠️ The `.app` must stay beside its sibling `Engine/` and `Stormfall/` folders.
`RunUAT -archive` copies only the `.app` and drops those siblings, producing a
400 MB shell that dies at `ICUInternationalization.cpp`. `Tools/build_game.sh`
now packages the whole staged directory instead. Moving the `.app` on its own
will re-break it.

---

## 2. STRIKE PROTOCOL — Godot 4.7

```bash
/Applications/Godot.app/Contents/MacOS/Godot --path ~/dev/Opus-5-Three-Games/02-strike-godot
```

Or open the project in Godot and press F5. Starts at the main menu.

| Input | Action |
|---|---|
| `WASD` / mouse | Move / look |
| `Shift` | Walk (silent, accurate) |
| `Ctrl` | Crouch |
| `Space` | Jump |
| **LMB** | Fire |
| `R` | Reload |
| `B` | Buy menu (during freeze time) — then `1`-`8` to purchase |
| `4` / `5` / `6` | Throw HE / smoke / flash |
| `Esc` | Pause |
| Arrows + `Enter` | Menus |

**What to try:** counter-strafe — tap the opposite key to stop dead before
shooting. Firing while moving is heavily penalised; standing still and
crouching are the accurate states. Recoil is a fixed learnable pattern, so
pull down and counter-sweep.

MR6: first to 7 rounds, sides swap at 6.

---

## 3. TEACUP — Godot 4.7

```bash
/Applications/Godot.app/Contents/MacOS/Godot --path ~/dev/Opus-5-Three-Games/03-teacup-godot
```

| Input | Action |
|---|---|
| `A`/`D` or arrows | Move |
| `W`/`S` + direction | Aim (8-directional) |
| `Shift` | Lock aim (stand still and aim freely) |
| `Space` | Jump |
| `X` or LMB | Shoot |
| `Z` | Dash (i-frames) |
| `C` | **Parry** — only works on pink projectiles |
| `B` | EX shot (costs 1/4 meter) |
| `V` | Super (costs full meter) |

The run opens with a **run-and-gun platforming stage** — reach the golden marker
past five turrets — then goes into the three bosses. **BOSS SELECT** on the main
menu skips the opener and jumps straight to any boss.

Three bosses, three phases each. Parrying pink is the only way to build meter,
and four parries fill the bar exactly. Death retries in under a second.

---

## Running the tests

```bash
# STORMFALL: 13 unit tests
cd ~/dev/Opus-5-Three-Games/01-stormfall-ue && ./Tools/run_tests.sh

# STORMFALL: 27 in-world integration checks
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "$PWD/Stormfall.uproject" -run=pythonscript \
  -script="$PWD/Tools/test_build_integration.py" -unattended -nosplash
cat /tmp/sf_integration_out.txt

# STRIKE: 148 assertions
/Applications/Godot.app/Contents/MacOS/Godot --headless \
  --path ~/dev/Opus-5-Three-Games/02-strike-godot --script res://tests/run_tests.gd

# STRIKE: full match simulation
/Applications/Godot.app/Contents/MacOS/Godot --headless \
  --path ~/dev/Opus-5-Three-Games/02-strike-godot res://scenes/sim.tscn

# TEACUP: 87 assertions
/Applications/Godot.app/Contents/MacOS/Godot --headless \
  --path ~/dev/Opus-5-Three-Games/03-teacup-godot --script res://tests/run_tests.gd

# TEACUP: boss-rush simulation
/Applications/Godot.app/Contents/MacOS/Godot --headless \
  --path ~/dev/Opus-5-Three-Games/03-teacup-godot res://scenes/sim.tscn
```

All green as of the last run: **13 + 148 + 87 = 248 assertions**, plus 27
in-world checks and two full-game simulations.

---

## What is verified, and what is not

**Verified by reproducible test output:** the rules, the maths, and that each
game runs. STRIKE plays a full 12-round match with real attrition and bomb
plants. TEACUP defeats all three bosses through all three phases. STORMFALL's
building, storm, loot, harvesting and combat all work in-world.

**Frame rate — now measured**, rendering on the Studio's M3 Ultra at 1280x720:

| Game | Mean FPS | Min FPS |
|---|---|---|
| STRIKE PROTOCOL | 117.2 | 109 |
| TEACUP | 118.7 | 115 |

Both roughly double the 60 fps target. STORMFALL now has a working packaged
build that boots the engine cleanly, but its framerate is still unmeasured --
it needs a human at the keyboard to load a match.

Frames are rendered and inspected via `tests/capture.gd` in each Godot project:

```bash
/Applications/Godot.app/Contents/MacOS/Godot --path <game> res://scenes/capture.tscn
# writes PNGs to ~/Library/Application Support/Godot/app_userdata/<NAME>/
```

**Still not verified — needs hands on a keyboard:**

- Whether any of it *feels* good. That is the whole question and no test answers it.
- Whether the bots are fun opponents rather than merely functional ones.
- STORMFALL end-to-end: no human has played a match to Victory Royale.

**Known tuning gaps:**

- STRIKE: bomb plants happen but are uncommon. Ts now reach the sites (the
  closest a T got went from 1764 units to 85, against a 500-unit plant radius)
  and the whole plant/defuse loop works, but most rounds still end by
  elimination. Note the simulation's player stands still, so its scoreline is
  4 T bots vs 5 CT bots -- a human fills that slot.
- STRIKE: bomb plants happen but remain uncommon; most rounds end by elimination.
