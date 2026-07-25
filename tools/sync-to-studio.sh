#!/usr/bin/env bash
# Push a game directory to the Mac Studio, where all builds and playtests run.
#
#   tools/sync-to-studio.sh 01-stormfall-ue
#
# Why this exists rather than a bare rsync: `rsync -a` preserves the local mtimes,
# which are often OLDER than the build artifacts already on the Studio. UnrealBuildTool
# then decides everything is up to date and silently runs a stale binary — a green
# test run that proves nothing. Touching the sources after the copy prevents that.
set -euo pipefail

STUDIO="${STUDIO:-midir@192.168.1.157}"
REMOTE_ROOT="${REMOTE_ROOT:-~/dev/Opus-5-Three-Games}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <game-dir>" >&2
  exit 2
fi
GAME_DIR="$1"

rsync -a --delete \
  --exclude 'Binaries/' --exclude 'Intermediate/' --exclude 'Saved/' \
  --exclude 'DerivedDataCache/' --exclude 'Content/' --exclude '.godot/' \
  --exclude 'assets/generated/' \
  "$REPO_ROOT/$GAME_DIR/" "$STUDIO:$REMOTE_ROOT/$GAME_DIR/"

# Force UBT/Godot to see the sources as newer than any existing artifacts.
ssh "$STUDIO" "cd $REMOTE_ROOT/$GAME_DIR && \
  find . -type f \\( -name '*.cpp' -o -name '*.h' -o -name '*.cs' -o -name '*.gd' \\) \
  -not -path './Intermediate/*' | xargs -r touch"

echo "==> synced $GAME_DIR to $STUDIO (sources touched)"
