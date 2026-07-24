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

# Regenerate every image asset whose file is absent.
python3 - "$REPO_ROOT" "$GAME_DIR" <<'PY' | while IFS=$'\t' read -r id prompt; do
import json, sys
from pathlib import Path

repo, game = Path(sys.argv[1]), sys.argv[2]
base = repo / game
data = json.loads((base / "assets" / "manifest.json").read_text())
for a in data["assets"]:
    if a.get("tool") != "codex":
        continue          # meshes/audio are rebuilt by their own scripts
    if (base / a["path"]).exists():
        continue
    print(f"{a['id']}\t{a['prompt']}".replace("\n", " "))
PY
  echo "--- regenerating $id"
  "$REPO_ROOT/tools/artgen/gen.sh" "$GAME_DIR" "$id" "$prompt"
done

echo
echo "==> after rebuild"
python3 "$REPO_ROOT/tools/artgen/manifest.py" check "$GAME_DIR"
