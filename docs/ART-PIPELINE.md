# Art pipeline

Every asset in this project is generated. Nothing is committed. The `manifest.json` in each game's
`assets/` directory is the source of truth — it is what makes "gitignore all binaries" recoverable.

## 1. 2D — Codex CLI image generation

Codex CLI (MacBook, v0.131.0) generates images through the logged-in ChatGPT account. No API key.
Images are written to `~/.codex/generated_images/<session-uuid>/*.png`.

```bash
tools/artgen/gen.sh <game-dir> <asset-id> "<prompt>"

# e.g.
tools/artgen/gen.sh 03-teacup-godot boss1_diffuse \
  "1930s rubber-hose cartoon texture sheet, a grinning bottle-cap boss, ink outlines, sepia palette, flat lighting"
```

`gen.sh` timestamps, invokes `codex exec`, harvests any PNG newer than that timestamp, writes it to
`<game-dir>/assets/generated/<asset-id>.png`, and appends the prompt + path + sha256 to the manifest.

Good for: textures, sprite sheets, skyboxes, UI, weapon icons, concept art, decals, HUD elements.
Not good for: anything needing exact tiling, precise alpha, or consistency across a large set — expect to
regenerate and cherry-pick.

## 2. 3D — headless Blender

Blender lives on the Studio. Mesh authoring is scripted, not hand-modeled:

```bash
/Applications/Blender.app/Contents/MacOS/Blender -b -P tools/blender/<script>.py -- <args>
```

Scripts should build geometry procedurally, apply the Codex-generated textures from step 1, and export
`.glb` (Godot) or `.fbx` (Unreal) into the target game's `assets/generated/`. Every script takes its
parameters as CLI args so a mesh is reproducible from the manifest entry.

## 3. Audio

Synthesized or procedural, or CC0-licensed. No copyrighted tracks — this is going on YouTube.
CC0 sources get recorded in the manifest with their URL and license instead of a prompt.

## 4. The manifest

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

`kind` is `image` | `mesh` | `audio`. For `tool: "blender"`, `prompt` is replaced by `script` + `args`.
For CC0 audio, `tool: "external"` with `source_url` and `license`.

## 5. Regenerating from a fresh clone

```bash
tools/artgen/rebuild.sh <game-dir>
```

Walks the manifest, regenerates anything missing, and reports hash mismatches. Generative output is not
deterministic — a mismatch means "this looks different now", not "this is broken". Review before accepting.

## Syncing to the Studio

Art generates on the MacBook; STORMFALL builds on the Studio:

```bash
rsync -av 01-stormfall-ue/assets/ midir@192.168.1.157:~/dev/Opus-5-Three-Games/01-stormfall-ue/assets/
```
