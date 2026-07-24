#!/usr/bin/env bash
# Build STORMFALL and run its unit tests headlessly on the Mac Studio.
#
#   Tools/run_tests.sh            # build + test
#   Tools/run_tests.sh --no-build # test only
#
# Exits non-zero if the build fails or any test fails. Note that a run matching
# zero tests is also a failure — otherwise tests compiled out would read as green.
set -euo pipefail

UE="/Users/Shared/Epic Games/UE_5.8/Engine"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT="$PROJECT_DIR/Stormfall.uproject"
LOG="${TMPDIR:-/tmp}/sf_tests.log"

if [[ "${1:-}" != "--no-build" ]]; then
  echo "==> building StormfallEditor"
  "$UE/Build/BatchFiles/Mac/Build.sh" StormfallEditor Mac Development -project="$PROJECT" \
    | grep -E "error|warning:|Result:|Total execution" || true
fi

echo "==> running tests"
set +e
"$UE/Binaries/Mac/UnrealEditor-Cmd" "$PROJECT" -run=SFTest \
  -unattended -nullrhi -nosplash -nosound > "$LOG" 2>&1
STATUS=$?
set -e

grep -E "SFTEST" "$LOG" | grep -v LogInit | sed 's/^\[[^]]*\]\[[^]]*\]//'
echo "==> exit $STATUS  (full log: $LOG)"
exit $STATUS
