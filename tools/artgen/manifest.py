#!/usr/bin/env python3
"""Read/write a game's assets/manifest.json.

    manifest.py record   # env-driven, called by gen.sh
    manifest.py check <game-dir>   # report missing files and hash drift
"""
from __future__ import annotations

import hashlib
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load(manifest: Path) -> dict:
    if manifest.exists():
        return json.loads(manifest.read_text())
    return {"version": 1, "assets": []}


def save(manifest: Path, data: dict) -> None:
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(json.dumps(data, indent=2) + "\n")


def record() -> int:
    """Upsert the asset gen.sh just produced. Parameters arrive via env."""
    manifest = Path(os.environ["MANIFEST"])
    game_dir = os.environ["GAME_DIR"]
    dest = Path(os.environ["DEST"])

    entry = {
        "id": os.environ["ASSET_ID"],
        "kind": "image",
        "tool": "codex",
        "prompt": os.environ["PROMPT"],
        "path": str(dest.relative_to(REPO_ROOT / game_dir)),
        "sha256": sha256(dest),
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
    }

    data = load(manifest)
    assets = [a for a in data["assets"] if a["id"] != entry["id"]]
    assets.append(entry)
    data["assets"] = sorted(assets, key=lambda a: a["id"])
    save(manifest, data)
    print(f"==> manifest updated: {entry['id']} ({entry['sha256'][:12]})")
    return 0


def check(game_dir: str) -> int:
    """Report which assets are missing or have drifted from their recorded hash."""
    base = REPO_ROOT / game_dir
    data = load(base / "assets" / "manifest.json")

    missing, drifted = [], []
    for asset in data["assets"]:
        path = base / asset["path"]
        if not path.exists():
            missing.append(asset)
        elif sha256(path) != asset["sha256"]:
            drifted.append(asset)

    for asset in missing:
        print(f"MISSING  {asset['id']:<28} {asset['path']}")
    for asset in drifted:
        print(f"DRIFTED  {asset['id']:<28} {asset['path']}")

    total = len(data["assets"])
    print(f"\n{total} tracked, {len(missing)} missing, {len(drifted)} drifted")
    # Drift is expected — generation is not deterministic. Only missing is a failure.
    return 1 if missing else 0


def main(argv: list[str]) -> int:
    if len(argv) >= 2 and argv[1] == "record":
        return record()
    if len(argv) >= 3 and argv[1] == "check":
        return check(argv[2])
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
