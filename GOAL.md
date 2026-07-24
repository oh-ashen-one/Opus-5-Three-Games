# GOAL — Opus 5 Three Games

> Paste this file's contents into a fresh Claude Code session to start a build.
> Say which game you want built first, or say "start at Game 1".

Build three playable games from scratch. Each must deliver **~10 minutes of real gameplay in one
sitting**, with a genuine end state, and be replayable. No browser games. No placeholder-only builds.
Work on one game at a time, in the listed order, and do not start the next until the current one meets
its Definition of Done.

Read `docs/ENVIRONMENT.md` before touching anything — it has the verified machine, engine, and
toolchain facts, including which machine each game gets built on.

---

## Ground rules (all three)

- **Single-player vs bots.** No netcode. Bots must be good enough to be genuinely fun to beat.
- **Feel before content.** Movement, shooting, and hit feedback get tuned to feeling good *before* any
  level, menu, or extra content is built. If the core loop isn't fun in a grey box, more content won't save it.
- **Playable at every commit.** Never leave the repo in a state that doesn't launch.
- **Test-driven where it's testable** — damage math, economy, storm timing, state machines, and bot
  decision logic all get unit tests (GoogleTest for UE, GUT for Godot). Feel and visuals get playtested,
  not asserted.
- **Original IP.** These are *recreations of mechanics*, not asset rips or trademark use. No Epic / Valve /
  Studio MDHR assets, names, logos, maps, or character likenesses. Use the codenames below.
- **Performance target:** 60 fps at 1440p on the Mac Studio (M3 Ultra); 60 fps at 1080p on the M5 MacBook.
- **Every game ships with:** a main menu, a settings screen (sensitivity, volume, invert-Y), pause, a win
  screen, a lose screen, and a working quit. Missing UI is the #1 way a build reads as unfinished on camera.

---

## Game 1 — **STORMFALL**
**Unreal Engine 5.8 · Mac Studio · `01-stormfall-ue/` · branch `game/stormfall`**
*Battle royale, third person. The Fortnite one.*

**The 10 minutes:** one full match. You drop onto an island with 15 bots, loot, fight, build, survive five
shrinking storm phases, and either win or die. Target match length 8–10 minutes.

### Must have
- Third-person character controller: sprint, crouch, mantle, slide, fall damage.
- **Deploy phase** — glider drop from a fixed flight path, player-chosen landing spot.
- **Loot:** ground spawns + breakable chests. 5 rarity tiers with real stat deltas. ~8 weapons (AR, shotgun,
  SMG, sniper, pistol) plus shields and heals. 5-slot inventory with hotkey swapping.
- **Building** — the mechanic that makes it a Fortnite and not just a BR. Harvest wood/stone/metal from the
  environment with a pickaxe; place wall / ramp / floor / roof on a world grid; structures have
  material-tiered HP and can be destroyed. Instant placement, no build delay.
- **Storm:** 5 phases, shrinking circle, escalating tick damage, on-screen timer and minimap circle.
- **Bots:** loot on landing, rotate ahead of the storm, engage on line-of-sight, take cover, panic-build a
  wall when shot. They must lose *believably*.
- **Island:** ~1 km², 6–8 distinct named POIs with real interiors and verticality. Generated via Editor
  Python commandlets, not hand-clicked.
- Kill feed, player counter, elimination popups, **Victory Royale** screen.

### Tech notes
Gameplay in **C++**; Blueprints only for thin data/UI glue. Level and asset population via
`UnrealEditor-Cmd -run=pythonscript`. Bots on Behavior Trees + EQS. Build headless with
`"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh"`. Watch the disk ceiling on the
Studio — cap the DDC and prune `Saved/` between iterations.

### Definition of Done
A full match played start to Victory Royale without a crash, at 60 fps, on camera.

---

## Game 2 — **STRIKE PROTOCOL**
**Godot 4.7 (3D) · MacBook · `02-strike-godot/` · branch `game/strike`**
*Tactical FPS, bomb defusal. The CS:GO one.*

**The 10 minutes:** one MR6 match (first to 7 rounds) on a single map — you + 4 bots vs 5 bots.
~1:55 rounds → 8–13 rounds → roughly 10 minutes.

### Must have
- **CS movement, exactly:** no acceleration curves, counter-strafing, a hard speed cap, air-strafing that
  works, and **accuracy penalties while moving and jumping**. This is the entire skill ceiling — get it
  right or nothing else matters.
- **Weapons with spray patterns:** AK-analog, M4-analog, AWP-analog, Deagle, two pistols, an SMG. Fixed
  learnable recoil patterns, first-shot accuracy, damage falloff, armor penetration, **4× headshot
  multiplier**, real reload timings.
- **Grenades:** HE, flash (with occlusion-checked blind duration), smoke (that actually blocks bot vision),
  molotov.
- **Economy:** kill rewards, round-loss bonuses, a buy menu on a buy-time window, dropped-weapon pickup.
  Bots must eco and force-buy.
- **Bomb loop:** T's carry and plant on A or B (3s plant), 40s fuse, 5s defuse / 10s without kit, defuse
  kits for CTs.
- **One map, built to compete:** two bombsites, mid control, connectors, and at least three genuinely
  different routes to each site. Blockout first, art pass second.
- **Bots:** navmesh pathing, per-round site executes, utility usage, retakes, crosshair placement, and a
  difficulty setting. They must hold angles, not run at you.
- Scoreboard, round/money/timer HUD, buy menu, half-time side swap, match-point → match-end screen.

### Tech notes
GDScript for gameplay; drop to C# or GDExtension only if profiling demands it. Godot's `CharacterBody3D`
defaults will **not** give you CS movement — write the controller from the ground up against unit tests for
velocity, counter-strafe, and air-strafe behavior.

### Definition of Done
A full MR6 match to 7 rounds, no crashes, bots a competent player has to actually try to beat.

---

## Game 3 — **TEACUP**
**Godot 4.7 (2.5D) · MacBook · `03-teacup-godot/` · branch `game/teacup`**
*Run-and-gun boss rush. The Cuphead one.*

**The 10 minutes:** a run-and-gun intro stage plus **three multi-phase bosses**. With retries — and there
will be retries — that's 10+ minutes.

### Must have
- **2.5D:** a fully 3D lit and rendered world with gameplay constrained to a 2D plane. Real depth,
  parallax, dynamic shadows. It must read as 3D on camera and play as 2D in your hands.
- **Combat:** 8-directional shooting, lock-aim, dash with i-frames, **parry on pink objects**, a super meter
  filled by parrying, EX moves, one super. 3 hit points, no health regen.
- **Three bosses, three phases each**, every phase with a distinct attack set and a visible tell. Phase
  transitions must be readable. Bosses telegraph — every death is the player's fault or it isn't shipping.
- **Instant retry** — under one second from death back into the fight. Non-negotiable for a game this hard.
- One run-and-gun platforming stage as the opener.
- **1930s rubber-hose aesthetic in 3D:** ink-outline + cel post-process shader, film grain, sepia grade,
  frame-gate jitter, vignette. Jazz soundtrack.
- Death counter, per-boss grade (A–F) on the results screen, boss select map.

### Tech notes
3D scene with a near-orthographic camera on a fixed rail. Bosses as hierarchical state machines with
data-driven attack tables — unit test the state machines, playtest the feel. Blender-authored boss meshes,
textures from the art pipeline below.

---

## Asset pipeline

No asset-store downloads, no manual steps. Generate on the MacBook, rsync to the Studio.
Full detail in `docs/ART-PIPELINE.md`.

1. **Images** (textures, sprites, skyboxes, UI, weapon icons) → **Codex CLI, and Codex is used for
   nothing else.** `tools/artgen/gen.sh <game-dir> <asset-id> "<prompt>"`. Verified working.
2. **Humanoid characters + animation** — solved per game, see below.
3. **Meshes** → headless Blender scripts in `tools/blender/`, textured with the images from step 1.
4. **Audio** → synthesized with numpy/scipy via `tools/audiogen/synth.py`. No sample packs, no
   copyrighted tracks.
5. **Every asset** gets a `assets/manifest.json` entry (prompt/script, path, sha256). Binaries stay
   gitignored; `tools/artgen/rebuild.sh` regenerates from the manifest.

### Humanoid animation — resolved, no downloads needed

- **STORMFALL:** use what Unreal already ships. `SKM_Manny_Simple` / `SKM_Quinn_Simple` plus **102
  animation sequences** in `Templates/TemplateResources/` — 8-direction walk and jog, aim offsets, ADS,
  fire, reload, equip, jump, directional hit reactions, 6 deaths, for both rifle and pistol — plus weapon
  meshes and shooter anim blueprints. Migrate and retarget. Do not hand-author locomotion.
  ⚠️ UE-EULA content: **Unreal projects only, never exported to the Godot games.**
- **STRIKE PROTOCOL:** it's first-person, so the player only ever sees a viewmodel — weapon transform
  animation, written in code, no rig needed. Bots get a Blender-scripted low-poly humanoid with
  procedurally keyframed walk/run/crouch cycles, and **die by ragdoll** (`PhysicalBone3D` + impulse)
  rather than by animation.
- **TEACUP:** nothing in it is humanoid. Rubber-hose animation is sine-driven limb rotation — 100%
  Blender-scripted.

---

## Working agreement

- **Report honestly.** If a mechanic doesn't work, say so with the evidence. A broken claim on camera is
  worse than a missing feature.
- Prefer a smaller thing that's genuinely fun over a larger thing that's technically complete.
- Commit early and often, on the branch listed for each game.
- **Flag it immediately if Studio free disk drops below 30 GB.**

---

## Nothing is blocked

Every asset class has a verified answer and no open questions remain. Start at Game 1.

The one thing to watch is **Studio free disk (~122 GB)** — a UE project plus DDC will take 30–60 GB of
it. Cap the DDC, prune `Saved/` between iterations, and raise a flag below 30 GB.
