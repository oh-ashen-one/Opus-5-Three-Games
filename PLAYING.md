# Playing all three

Everything runs on the Mac Studio (`midir@192.168.1.157`), repo at
`~/dev/Opus-5-Three-Games`.

---

## 1. STORMFALL — Unreal Engine 5.8

```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" \
  ~/dev/Opus-5-Three-Games/01-stormfall-ue/Stormfall.uproject
```

Opens on `Lvl_Island`. Press **Play → Standalone Game** for a clean view.

| Input | Action |
|---|---|
| `WASD` / mouse | Move / look |
| `Shift` / `Ctrl` / `Space` | Sprint / crouch / jump |
| **LMB** | Fire — also harvests: shoot trees and rocks for materials |
| `R` | Reload |
| **`F`** | Place the selected build piece |
| `Q` | Cycle wall → ramp → floor → roof |
| `Esc` | Pause |

Maps: `Lvl_Island` (the match — 8 POIs, 15 bots, storm) and `Lvl_Greybox`
(movement tuning, drop-test ledges).

⚠️ **The packaged `.app` does not launch** — see `01-stormfall-ue/RUNNING.md`.
It is a path-layout problem specific to this machine, not a game bug. Editor
play is unaffected.

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
| `B` | Buy menu (during freeze time) |
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

**Not verified — needs hands on a keyboard:**

- Whether any of it *feels* good. That is the whole question and no test answers it.
- Frame rate. Nothing has been profiled; the 60 fps target is unmeasured.
- Whether the bots are fun opponents rather than merely functional ones.
- STORMFALL end-to-end: no human has played a match to Victory Royale.

**Known tuning gaps:**

- STRIKE: only ~1 bomb plant per 12 rounds. The mechanic works end to end but
  Ts rarely survive long enough to use it; the bots trade too readily.
- STRIKE: bots hold angles and shoot, but have no utility usage (smokes and
  flashes exist as rules, not as bot behaviour).
- TEACUP: bosses are spheres. The rubber-hose look is in the lighting, grade and
  palette, not yet in the geometry.
