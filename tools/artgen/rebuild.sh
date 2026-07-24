#!/usr/bin/env bash
# Regenerate a game's missing assets from its manifest, after a fresh clone.
#
#   tools/artgen/rebuild.sh <game-dir>
#
# Generation is not deterministic — regenerated art will differ from the recorded
# hashes. That is expected; review the output before committing the new manifest.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <game-dir>" >&2
  exit 2
fi
GAME_DIR="$1"
MANIFEST="$REPO_ROOT/$GAME_DIR/assets/manifest.json"

[[ -f "$MANIFEST" ]] || { echo "no manifest at $MANIFEST" >&2; exit 1; }

echo "==> current state"
python3 "$REPO_ROOT/tools/artgen/manifest.py" check "$GAME_DIR" || true
echo

# Regenerate every missing asset. Blender meshes are skipped — rerun their scripts by hand,
# since they may depend on textures that this pass is still generating.
python3 - "$REPO_ROOT" "$GAME_DIR" <<'PY' | while IFS=$'\t' read -r tool id rest; do
import json, sys
from pathlib import Path

repo, game = Path(sys.argv[1]), sys.argv[2]
base = repo / game
data = json.loads((base / "assets" / "manifest.json").read_text())
for a in data["assets"]:
    if (base / a["path"]).exists():
        continue
    tool = a.get("tool")
    if tool == "codex":
        rest = a["prompt"]
    elif tool == "audiogen":
        rest = " ".join(a["args"])
    else:
        print(f"skip\t{a['id']}\t{tool} asset must be rebuilt manually")
        continue
    print(f"{tool}\t{a['id']}\t{rest}".replace("\n", " "))
PY
  case "$tool" in
    codex)    echo "--- regenerating image $id"
              "$REPO_ROOT/tools/artgen/gen.sh" "$GAME_DIR" "$id" "$rest" ;;
    audiogen) echo "--- regenerating audio $id"
              # shellcheck disable=SC2086
              python3 "$REPO_ROOT/tools/audiogen/synth.py" "$GAME_DIR" "$id" $rest ;;
    skip)     echo "!!! $id — $rest" ;;
  esac
done

echo
echo "==> after rebuild"
python3 "$REPO_ROOT/tools/artgen/manifest.py" check "$GAME_DIR"
