#!/usr/bin/env bash
# Generate one image asset via Codex CLI and record it in the game's manifest.
#
#   tools/artgen/gen.sh <game-dir> <asset-id> "<prompt>"
#
# Codex writes images to ~/.codex/generated_images/<session>/*.png. We stamp the
# time before invoking it and harvest whatever appeared after that stamp.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CODEX_IMAGES="${CODEX_IMAGES:-$HOME/.codex/generated_images}"

if [[ $# -lt 3 ]]; then
  echo "usage: $0 <game-dir> <asset-id> \"<prompt>\"" >&2
  exit 2
fi

GAME_DIR="$1"; ASSET_ID="$2"; PROMPT="$3"

command -v codex >/dev/null 2>&1 || {
  echo "error: codex CLI not found. Art generation only runs on the MacBook." >&2
  exit 1
}

OUT_DIR="$REPO_ROOT/$GAME_DIR/assets/generated"
MANIFEST="$REPO_ROOT/$GAME_DIR/assets/manifest.json"
mkdir -p "$OUT_DIR" "$(dirname "$MANIFEST")"
[[ -f "$MANIFEST" ]] || echo '{"version": 1, "assets": []}' > "$MANIFEST"

STAMP="$(mktemp -t artgen-stamp)"
trap 'rm -f "$STAMP"' EXIT

echo "==> generating '$ASSET_ID'"
codex exec "Generate an image. Do not write any files or code — just produce the image.

$PROMPT" >/dev/null

# Newest PNG created after the stamp.
NEW_PNG="$(find "$CODEX_IMAGES" -type f -name '*.png' -newer "$STAMP" -print0 2>/dev/null \
  | xargs -0 ls -t 2>/dev/null | head -1 || true)"

if [[ -z "$NEW_PNG" ]]; then
  echo "error: codex produced no new image for '$ASSET_ID'." >&2
  echo "       check '$CODEX_IMAGES' and retry with a more explicit prompt." >&2
  exit 1
fi

DEST="$OUT_DIR/$ASSET_ID.png"
cp "$NEW_PNG" "$DEST"
echo "==> wrote $DEST"

REPO_ROOT="$REPO_ROOT" GAME_DIR="$GAME_DIR" ASSET_ID="$ASSET_ID" \
PROMPT="$PROMPT" MANIFEST="$MANIFEST" DEST="$DEST" \
python3 "$REPO_ROOT/tools/artgen/manifest.py" record
