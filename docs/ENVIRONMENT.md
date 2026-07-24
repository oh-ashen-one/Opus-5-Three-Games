# Environment

Verified 2026-07-24. Re-verify before assuming any of this still holds.

## Machines

### Mac Studio — `midir@192.168.1.157` (`midirstudio.local`)
Builds **STORMFALL**. Apple M3 Ultra, macOS 26.5.2.

| Thing | Value |
|---|---|
| Unreal Engine | **5.8** — `/Users/Shared/Epic Games/UE_5.8` (47 GB) |
| UE build script | `/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh` |
| UE editor | `/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor` |
| Godot | `/Applications/Godot.app/Contents/MacOS/Godot` — **not on `PATH`** |
| Blender | `/Applications/Blender.app` (headless: `Blender.app/Contents/MacOS/Blender -b -P script.py`) |
| Xcode | 26.6 |
| git | 2.50.1 — **`git-lfs` is NOT installed** |
| Codex CLI | **not installed** — art generation happens on the MacBook |
| Repo | `/Users/midir/dev/Opus-5-Three-Games` |

> ⚠️ **Disk: ~122 GB free of 1.8 TB.** A UE project plus its DerivedDataCache will claim 30–60 GB.
> Cap the DDC, prune `Saved/` between iterations, and raise a flag if free space drops below 30 GB.

### MacBook — `darkeatermidir@` local
Builds **STRIKE PROTOCOL** and **TEACUP**, and generates all art. Apple M5, 32 GB RAM, ~268 GB free.

| Thing | Value |
|---|---|
| Godot | 4.7.1 stable — `/opt/homebrew/bin/godot` |
| Xcode | 26.6 |
| Codex CLI | 0.131.0 — `~/.npm-global/bin/codex` |
| Python | 3.14.5 |

## Codex image generation

Authenticated through the ChatGPT account in `~/.codex/auth.json` (`OPENAI_API_KEY` is null —
**no API key needed**). Generated images land in `~/.codex/generated_images/<session-uuid>/*.png`;
114 were already on disk from previous sessions, so the mechanism is real.

> 🔴 **Currently broken — Codex CLI is out of date.** An end-to-end test of `tools/artgen/gen.sh`
> on 2026-07-24 failed with:
>
> ```
> ERROR: The 'gpt-5.6-sol' model requires a newer version of Codex.
>        Please upgrade to the latest app or CLI and try again.
> ```
>
> `~/.codex/config.toml` pins `model = "gpt-5.6-sol"`; installed CLI is **0.131.0**, npm has **0.145.0**.
> Fix before the first art pass:
>
> ```bash
> npm install -g @openai/codex@latest   # or: codex update
> ```
>
> Then re-run the end-to-end check:
> `tools/artgen/gen.sh 02-strike-godot _pipeline_test "a seamless tileable concrete wall texture"`
>
> (Unrelated noise in the same run: the Linear and Notion MCP servers configured for Codex have expired
> OAuth tokens. Harmless here.)

Codex is **MacBook-only**. Generate here, then rsync to the Studio:

```bash
rsync -av --progress <local-assets>/ midir@192.168.1.157:~/dev/Opus-5-Three-Games/<game>/assets/
```

## Toolchain smoke test

Run this before writing any game code:

```bash
# Studio
ssh midir@192.168.1.157 '"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" -help | head -5; \
  /Applications/Godot.app/Contents/MacOS/Godot --version; df -h / | tail -1'

# MacBook
godot --version && codex --version
```

## Known gaps

- **Codex CLI needs upgrading** before any art can be generated — see the red box above.
- **No humanoid animation source.** See the open decision at the end of `GOAL.md`.
- **No `git-lfs` on the Studio.** Intentional — binaries are gitignored, not versioned.
- **`godot` is not on the Studio's `PATH`.** Use the full `.app` binary path there.
