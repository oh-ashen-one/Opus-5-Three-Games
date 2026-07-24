# Running STORMFALL

All of this runs on the Mac Studio (`midir@192.168.1.157`).

## Play it (the way that works today)

Open the project in the Unreal Editor and press Play:

```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" \
  ~/dev/Opus-5-Three-Games/01-stormfall-ue/Stormfall.uproject
```

The editor opens on `Lvl_Island`. Hit **Play** (Alt+P) and select **New Editor Window** or
**Standalone Game** for a clean fullscreen view without editor overlays.

### Controls

| Input | Action |
|---|---|
| `WASD` | Move |
| Mouse | Look |
| `Shift` | Sprint |
| `Ctrl` | Crouch |
| `Space` | Jump |
| **LMB** | Fire (also harvests — shoot trees/rocks for materials) |
| `R` | Reload |
| **`F`** | Place the selected build piece |
| `Q` | Cycle piece: wall → ramp → floor → roof |
| `Esc` | Pause menu |
| Arrows + `Enter` | Navigate menus |

### Maps

- `Lvl_Island` — the real match: 8 POIs, 15 bots, storm, 518 actors
- `Lvl_Greybox` — movement tuning box with drop-test ledges at known heights

## Tests

```bash
Tools/run_tests.sh                  # build + 13 unit tests, exits non-zero on failure
```

In-world integration checks (27 of them — building, loot, damage, harvesting, storm routing):

```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "$PWD/Stormfall.uproject" -run=pythonscript \
  -script="$PWD/Tools/test_build_integration.py" -unattended -nosplash
cat /tmp/sf_integration_out.txt
```

## Regenerating the maps

Deterministic — same seed, same island:

```bash
UnrealEditor-Cmd Stormfall.uproject -run=pythonscript -script=Tools/gen_island.py
UnrealEditor-Cmd Stormfall.uproject -run=pythonscript -script=Tools/gen_greybox.py
```

---

## Known issue: the packaged build does not run

`Tools/build_game.sh` builds the standalone Game target, and `RunUAT BuildCookRun` cooks and
packages successfully (`BUILD SUCCESSFUL`, 411 MB in `Dist/Mac/Stormfall.app`). The resulting
`.app` **still does not launch**, for reasons specific to this machine's layout rather than to the
game code. Three separate faults, in the order they surface:

1. **Broken relative rpath.** UnrealBuildTool bakes nine `../` segments from `Binaries/Mac` to the
   engine's ThirdParty libraries. This project lives at
   `/Users/midir/dev/Opus-5-Three-Games/01-stormfall-ue`, so nine levels up overshoots `/Users`
   and resolves to `/Shared/Epic Games/…` instead of `/Users/Shared/Epic Games/…`. `dyld` then
   cannot find `libmetalirconverter.dylib` and the process dies before printing one line of its
   own log. `DYLD_LIBRARY_PATH` is no help — SIP strips `DYLD_*`.
   *Worked around* in `build_game.sh` by adding absolute rpaths with `install_name_tool` and
   re-signing (editing a Mach-O invalidates its signature).

2. **Incomplete stage.** The packaged bundle does not contain the engine's ThirdParty dylibs
   (`libtbb.12.dylib` and friends) or the engine's ICU internationalisation data, so once dyld is
   satisfied the app fails at `ICUInternationalization.cpp:161` — "ICU data directory was not
   discovered".

3. **Project descriptor path.** The same relative-path arithmetic misresolves the `.uproject`
   location from inside the bundle.

The common root cause is that the engine (`/Users/Shared/Epic Games/UE_5.8`) and this project
(`/Users/midir/dev/…`) sit on different branches of `/Users`, and UE's staging computes
binary-relative paths that assume a fixed depth.

**The likely fix, untried:** install the engine and project under a common ancestor — for example
move the repo to `/Users/Shared/dev/Opus-5-Three-Games` — so the relative path arithmetic lands
correctly. That is a disruptive change to make without asking, so it hasn't been done.

**None of this affects playing in the editor**, which is the normal way to playtest and works
today. It only matters for shipping a double-clickable build.

## What has not been verified

The engine's `-game` path cannot be driven headlessly on this machine (it short-circuits into
UnrealBuildTool and exits without booting), so the following are **untested** and need a human at
the keyboard:

- Whether the movement, shooting, and building actually *feel* good
- Whether the bots read as competent opponents
- Frame rate with 15 bots and 518 actors (target: 60 fps at 1440p)
- A complete match played through to Victory Royale

Every other claim in this repo is backed by a test whose output is reproducible.
