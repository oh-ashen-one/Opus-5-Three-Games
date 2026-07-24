Build three games from scratch, one at a time, in order. Each must deliver ~10 minutes of real,
replayable gameplay with a genuine end state. Do not start the next until the current one is Done.

**The full spec is `~/dev/Opus-5-Three-Games/GOAL.md` on the Mac Studio. Read it first, in full,
along with `docs/ENVIRONMENT.md`. It is authoritative — this is only the summary.**

1. **STORMFALL** — Unreal Engine 5.8, `01-stormfall-ue/`. Fortnite-like battle royale, 3rd person:
   glider drop, loot, shooting, building, shrinking storm, 15 bots, Victory Royale.
   Done = a full match played to Victory Royale, no crash, 60fps.
2. **STRIKE PROTOCOL** — Godot 4.7 3D, `02-strike-godot/`. CS:GO-like tactical FPS: true CS movement,
   spray patterns, utility, economy, bomb defusal, one competitive map, 5 bots vs 5.
   Done = a full MR6 match to 7 rounds, no crashes, bots you have to try to beat.
3. **TEACUP** — Godot 4.7 2.5D, `03-teacup-godot/`. Cuphead-like run-and-gun boss rush: 3D-rendered
   world on a 2D plane, parry, dash, 3 bosses × 3 phases, instant retry, 1930s rubber-hose look.
   Done = one stage + all three bosses beatable.

## Run everything on the Mac Studio
**All work happens on `midir@192.168.1.157` (M3 Ultra) — every build, render, asset generation, and
playtest. Nothing heavy runs on the MacBook.** The repo lives at `~/dev/Opus-5-Three-Games` there;
UE 5.8, Godot, and Blender are all installed. SSH in and work in place. The one exception is Codex
image generation, which is only authenticated on the MacBook — generate there, rsync to the Studio.
Watch Studio free disk (~122GB); UE + DDC will take 30–60GB. Flag below 30GB.

## Non-negotiables
Single-player vs bots, no netcode. Feel before content — tune movement and shooting in a grey box
before building levels. Playable at every commit. Unit-test the testable, playtest the feel. Original
IP only — no Epic/Valve/Studio MDHR assets, names, logos, maps, or likenesses. 60fps. Every game ships
a main menu, settings, pause, win, lose, and quit. Assets are all generated, never downloaded — see
the asset pipeline in `GOAL.md`. Commit often on the branch named for each game.

**Report honestly with evidence.** If a mechanic doesn't work, say so and show the output. A broken
claim on camera is worse than a missing feature. This is being filmed as an Opus 5 benchmark, so
prefer a smaller thing that is genuinely fun over a larger thing that merely compiles.
