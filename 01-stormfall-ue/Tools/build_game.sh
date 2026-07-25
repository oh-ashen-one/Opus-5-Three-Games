#!/usr/bin/env bash
# Build the standalone STORMFALL game target and make it actually launchable.
#
#   Tools/build_game.sh          # build + fix rpath + sign
#   Tools/build_game.sh --run    # ...then run it offscreen for a smoke test
#
# Why the rpath fixup exists:
#
# UnrealBuildTool bakes a *relative* rpath from Binaries/Mac to the engine's
# ThirdParty libraries. It emits nine "../" segments, which assumes the project
# sits deeper under the filesystem root than this one does. From
#   /Users/midir/dev/Opus-5-Three-Games/01-stormfall-ue/Binaries/Mac
# nine levels up overshoots /Users entirely and resolves to
#   /Shared/Epic Games/...        (instead of /Users/Shared/Epic Games/...)
# so dyld cannot find libmetalirconverter.dylib and the game dies before it
# prints a single line of its own log.
#
# DYLD_LIBRARY_PATH does not work around it — SIP strips DYLD_* from the
# environment. So we add an absolute rpath to the built binary instead, then
# re-sign it, because editing a Mach-O invalidates its signature.
set -euo pipefail

UE="/Users/Shared/Epic Games/UE_5.8/Engine"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT="$PROJECT_DIR/Stormfall.uproject"
BINARY="$PROJECT_DIR/Binaries/Mac/Stormfall"

THIRD_PARTY_RPATHS=(
  "$UE/Binaries/ThirdParty/Apple/MetalShaderConverter/Mac"
  "$UE/Binaries/ThirdParty/Ogg/Mac"
  "$UE/Binaries/ThirdParty/Vorbis/Mac"
  "$UE/Plugins/NNE/NNERuntimeORT/Binaries/ThirdParty/Onnxruntime/Mac"
  "$UE/Source/ThirdParty/Intel/TBB/Deploy/oneTBB-2022.3.0/Mac/lib"
)

echo "==> building Stormfall (Game target)"
BUILD_LOG="${TMPDIR:-/tmp}/sf_game_build.log"
set +e
"$UE/Build/BatchFiles/Mac/Build.sh" Stormfall Mac Development -project="$PROJECT" > "$BUILD_LOG" 2>&1
STATUS=$?
set -e
grep -E "error|Result:|Output binary" "$BUILD_LOG" || true
if [[ $STATUS -ne 0 ]]; then
  echo "==> BUILD FAILED (exit $STATUS). Log: $BUILD_LOG"
  exit $STATUS
fi

echo "==> patching rpaths"
for RP in "${THIRD_PARTY_RPATHS[@]}"; do
  # Already-present rpaths make install_name_tool fail; that is not an error.
  install_name_tool -add_rpath "$RP" "$BINARY" 2>/dev/null || true
done

echo "==> re-signing (editing the Mach-O invalidated the signature)"
codesign --force --sign - --timestamp=none "$BINARY" 2>/dev/null || true

echo "==> verifying dyld can resolve everything"
if ! otool -L "$BINARY" >/dev/null 2>&1; then
  echo "otool failed on $BINARY" >&2
  exit 1
fi

# ── Packaging ───────────────────────────────────────────────────────────────
#
# `RunUAT BuildCookRun -archive` copies ONLY Stormfall.app into the archive
# directory and drops its siblings. That is fatal: a staged Mac build is the
# .app PLUS sibling Engine/ and Stormfall/Content/Paks/ directories, and without
# them the app cannot find its content, its ICU data, or its own .uproject. The
# resulting bundle is a 400MB shell that dies at ICUInternationalization.cpp.
#
# The real staged build is in Saved/StagedBuilds/Mac. Copy that whole directory.
STAGE_DIR="$PROJECT_DIR/Saved/StagedBuilds/Mac"
DIST_DIR="$PROJECT_DIR/Dist/Mac"
if [[ -d "$STAGE_DIR" ]]; then
  echo "==> packaging from the staged build (app + siblings)"
  rm -rf "$DIST_DIR"
  mkdir -p "$PROJECT_DIR/Dist"
  cp -R "$STAGE_DIR/" "$DIST_DIR/"

  APP_BINARY="$DIST_DIR/Stormfall.app/Contents/MacOS/Stormfall"
  if [[ -f "$APP_BINARY" ]]; then
    echo "==> patching packaged app rpaths"
    for RP in "${THIRD_PARTY_RPATHS[@]}"; do
      install_name_tool -add_rpath "$RP" "$APP_BINARY" 2>/dev/null || true
    done
    codesign --force --deep --sign - --timestamp=none \
      "$DIST_DIR/Stormfall.app" 2>/dev/null || true
  fi
  echo "==> playable build: $DIST_DIR/Stormfall.app"
fi

if [[ "${1:-}" == "--run" ]]; then
  LOG="${TMPDIR:-/tmp}/sf_smoke.log"
  rm -rf "$PROJECT_DIR/Saved/Logs"
  echo "==> smoke running offscreen"
  "$BINARY" /Game/Maps/Lvl_Island -RenderOffScreen -unattended -nosound \
    -ResX=1280 -ResY=720 -ExecCmds="stat unit" > "$LOG" 2>&1 &
  GAME_PID=$!
  # Give it time to load the map and spawn the field.
  for _ in $(seq 1 60); do
    sleep 2
    if [[ -f "$PROJECT_DIR/Saved/Logs/Stormfall.log" ]] && \
       grep -qiE "Bringing World|Fatal error" "$PROJECT_DIR/Saved/Logs/Stormfall.log"; then
      break
    fi
    kill -0 "$GAME_PID" 2>/dev/null || break
  done
  sleep 8
  kill "$GAME_PID" 2>/dev/null || true
  wait "$GAME_PID" 2>/dev/null || true
  echo "==> smoke log: $PROJECT_DIR/Saved/Logs/Stormfall.log"
fi

echo "==> done: $BINARY"
