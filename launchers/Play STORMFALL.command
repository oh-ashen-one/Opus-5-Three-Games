#!/bin/bash
# Double-click to open STORMFALL in the Unreal Editor, then press Play (Alt+P).
#
# The packaged .app does not launch on this machine: UE 5.8's macOS staging
# computes the project path relative to the binary assuming it sits at
# <Project>/Binaries/Mac, but a staged .app puts it five levels deep inside the
# bundle. The path it derives is wrong no matter where the .app is placed --
# verified by cooking from three different locations. Editor play is unaffected.
open -a "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app" \
  --args "$HOME/dev/Opus-5-Three-Games/01-stormfall-ue/Stormfall.uproject" 2>/dev/null \
  || "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor" \
     "$HOME/dev/Opus-5-Three-Games/01-stormfall-ue/Stormfall.uproject"
