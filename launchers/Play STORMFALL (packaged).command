#!/bin/bash
# Double-click to play the packaged STORMFALL build -- no editor needed.
#
# The .app must stay next to its sibling Engine/ and Stormfall/ folders; moving
# the .app on its own breaks it, which is exactly the bug that made the archived
# build unplayable.
exec "$HOME/dev/Opus-5-Three-Games/01-stormfall-ue/Dist/Mac/Stormfall.app/Contents/MacOS/Stormfall"
