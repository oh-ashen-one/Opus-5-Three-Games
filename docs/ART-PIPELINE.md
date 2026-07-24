# Asset pipeline

Nothing is committed. The `manifest.json` in each game's `assets/` directory is the source of truth —
it is what makes "gitignore all binaries" recoverable.

**Codex is used for image generation only.** Every other asset class has its own answer below. There are
no manual downloads and no external asset stores anywhere in this pipeline.

| Asset class | STORMFALL (UE) | STRIKE PROTOCOL (Godot) | TEACUP (Godot) |
|---|---|---|---|
| Textures / UI / skyboxes | Codex | Codex | Codex |
| Humanoid meshes + animation | **UE bundled Manny/Quinn** | Blender-scripted + ragdoll | n/a |
| Props / weapons / bosses | UE bundled weapons + Blender | Blender-scripted | Blender-scripted |
| Levels | UE Editor Python commandlets | Godot scene scripts | Godot scene scripts |
| Audio | numpy synthesis | numpy synthesis | numpy synthesis |

---

## 1. Images — Codex CLI

Codex (MacBook, **0.145.0**) generates images through the logged-in ChatGPT account. No API key.
Output lands in `~/.codex/generated_images/<session-uuid>/*.png`.

```bash
tools/artgen/gen.sh <game-dir> <asset-id> "<prompt>"

# e.g.
tools/artgen/gen.sh 03-teacup-godot boss1_diffuse \
  "1930s rubber-hose cartoon texture sheet, a grinning bottle-cap boss, ink outlines, sepia palette, flat lighting"
```

`gen.sh` stamps the time, invokes `codex exec`, harvests any PNG newer than that stamp, writes it to
`<game-dir>/assets/generated/<asset-id>.png`, and records prompt + path + sha256 in the manifest.

**Good for:** tileable textures, sprite sheets, skyboxes, UI, weapon icons, decals, HUD, concept art.
**Weak at:** exact tiling guarantees, clean alpha, and consistency across a large set. Expect to
regenerate and cherry-pick. Ask for "flat even lighting, no shadows, orthographic view of the surface"
on anything that will be used as a texture — baked-in lighting is the usual failure.

---

## 2. Humanoid characters and animation

This is the asset class with no generative answer, so each game solves it differently.

### STORMFALL — use what Unreal ships

UE 5.8 bundles `SKM_Manny_Simple` / `SKM_Quinn_Simple` plus **102 animation sequences** covering
8-direction walk and jog, aim offsets, ADS, fire, reload, equip, jump, directional hit reactions, and 6
deaths — for both rifle and pistol. Plus weapon meshes and shooter anim blueprints. Paths are listed in
`docs/ENVIRONMENT.md`. Migrate them into the project and retarget; do not author locomotion by hand.

> ⚠️ Licensed by the UE EULA for **Unreal projects only**. Never export this content to the Godot games.

### STRIKE PROTOCOL — first-person hides the problem

- **The player never sees a full body.** Only a viewmodel: arms + weapon. Weapon animation (fire recoil,
  reload, draw, inspect, bob, sway) is **transform animation on the weapon node**, keyframed in Godot's
  AnimationPlayer or driven procedurally in code. No mocap involved, and it's how CS viewmodels actually
  read anyway.
- **Bots need bodies**, but at engagement distance and in a stylized art style, a low-poly rigged
  humanoid with procedurally-authored cycles is enough. `tools/blender/` generates the mesh and rig, and
  keyframes walk / run / crouch-walk / idle cycles from joint-rotation curves.
- **Death is ragdoll, not animation.** Godot `PhysicalBone3D` + impulse from the killing shot. This
  removes the need for death animations entirely and looks better than a canned clip.

### TEACUP — nothing here is humanoid

Bosses are cartoon objects — a bottle cap, a teapot, a gramophone. Rubber-hose animation is *literally*
sine-driven limb rotation, which is the easiest thing in the world to script. 100% Blender-authored,
keyframed procedurally, exported to `.glb`. The player character is small and stylized enough for the
same treatment.

---

## 3. Meshes — headless Blender

Blender 5.2 LTS on the Studio. Mesh authoring is scripted, never hand-modeled:

```bash
/Applications/Blender.app/Contents/MacOS/Blender -b -P tools/blender/<script>.py -- <args>
```

Conventions:

- Parameters come in as CLI args after `--`, so every mesh is reproducible from its manifest entry.
- Build geometry procedurally; apply the Codex textures from step 1.
- Export `.glb` for Godot, `.fbx` for Unreal. Z-up → engine conversion is the script's job.
- Record with `"tool": "blender"`, plus `script` and `args` in place of `prompt`.
- Keep polycounts honest — these have to hold 60 fps.

---

## 4. Audio — synthesized

No sample packs, no copyrighted tracks. Everything is generated with **numpy only** — no scipy, so it
runs on the Studio's Python 3.9 too. `ffmpeg` (MacBook only) is for re-encoding, not synthesis.

```bash
tools/audiogen/synth.py <game-dir> <asset-id> <kind> [--params …]
```

`tools/audiogen/synth.py` provides the primitives that cover almost every game sound: filtered noise
bursts with ADSR envelopes (gunshots, impacts, footsteps), pitched oscillators (UI, pickups, alerts),
and low-frequency sweeps (explosions, the storm). Weapon character comes from filter cutoff, envelope
shape, and layering — not from sample libraries.

TEACUP's jazz soundtrack is the one genuinely hard piece: additive synthesis with swung timing over a
chord progression. Budget real time for it, and treat CC0 as the fallback if it doesn't sound good
enough — a bad soundtrack is worse than a borrowed one.

---

## 5. The manifest

`<game-dir>/assets/manifest.json`:

```json
{
  "version": 1,
  "assets": [
    {
      "id": "boss1_diffuse",
      "kind": "image",
      "tool": "codex",
      "prompt": "1930s rubber-hose cartoon texture sheet, ...",
      "path": "assets/generated/boss1_diffuse.png",
      "sha256": "…",
      "generated_at": "2026-07-24T12:00:00Z"
    }
  ]
}
```

`kind` is `image` | `mesh` | `audio`. `tool` is `codex` | `blender` | `audiogen` | `ue-bundled`.
For `blender` and `audiogen`, `script` + `args` replace `prompt`. For `ue-bundled`, record the engine
source path so a reader can see exactly what was migrated and from where.

## 6. Regenerating from a fresh clone

```bash
tools/artgen/rebuild.sh <game-dir>
```

Regenerates anything missing and reports hash drift. Generation is not deterministic — drift means "this
looks different now", not "this is broken". Review before accepting.

## 7. Syncing to the Studio

Art and audio generate on the MacBook; STORMFALL builds on the Studio:

```bash
rsync -av 01-stormfall-ue/assets/ midir@192.168.1.157:~/dev/Opus-5-Three-Games/01-stormfall-ue/assets/
```
