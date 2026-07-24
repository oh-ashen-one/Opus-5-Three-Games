#!/usr/bin/env python3
"""Synthesize a game sound effect and record it in the game's manifest.

    tools/audiogen/synth.py <game-dir> <asset-id> <kind> [key=value ...]

Kinds: gunshot, impact, footstep, explosion, ui, pickup, whoosh

Everything is built from three primitives — filtered noise, pitched oscillators, and
frequency sweeps — shaped by an ADSR-ish envelope. Weapon character comes from filter
cutoff, envelope shape, and layering, not from sample libraries.

    # a punchy rifle
    synth.py 02-strike-godot wpn_rifle_fire gunshot cutoff=3200 decay=0.16 body=90
    # a heavier, boomier one
    synth.py 02-strike-godot wpn_awp_fire   gunshot cutoff=1800 decay=0.42 body=55

numpy only — no scipy, so this runs on both machines.
"""
from __future__ import annotations

import math
import sys
import wave
from datetime import datetime, timezone
from pathlib import Path

import numpy as np

REPO_ROOT = Path(__file__).resolve().parents[2]
SR = 44100


# ── primitives ──────────────────────────────────────────────────────────────

def env(n: int, attack: float, decay: float, curve: float = 2.5) -> np.ndarray:
    """Percussive envelope: near-instant attack, exponential decay."""
    a = max(1, int(attack * SR))
    d = max(1, n - a)
    return np.concatenate([
        np.linspace(0.0, 1.0, a),
        (1.0 - np.linspace(0.0, 1.0, d)) ** curve,
    ])[:n]


def noise(n: int, seed: int = 0) -> np.ndarray:
    return np.random.default_rng(seed).uniform(-1.0, 1.0, n)


def lowpass(x: np.ndarray, cutoff: float) -> np.ndarray:
    """One-pole IIR lowpass. Cheap, and plenty for SFX."""
    a = math.exp(-2.0 * math.pi * cutoff / SR)
    y = np.empty_like(x)
    prev = 0.0
    for i, v in enumerate(x):
        prev = (1.0 - a) * v + a * prev
        y[i] = prev
    return y


def highpass(x: np.ndarray, cutoff: float) -> np.ndarray:
    return x - lowpass(x, cutoff)


def sweep(n: int, f0: float, f1: float, shape: str = "exp") -> np.ndarray:
    """Sine whose frequency glides f0 → f1."""
    t = np.arange(n) / SR
    frac = np.linspace(0.0, 1.0, n)
    freq = f0 * (f1 / f0) ** frac if shape == "exp" else f0 + (f1 - f0) * frac
    return np.sin(2.0 * np.pi * np.cumsum(freq) / SR)


def normalize(x: np.ndarray, peak: float = 0.89) -> np.ndarray:
    m = np.max(np.abs(x))
    return x * (peak / m) if m > 0 else x


# ── voices ──────────────────────────────────────────────────────────────────

def gunshot(cutoff=3000.0, decay=0.20, body=80.0, crack=0.7, seed=1):
    """Transient crack + filtered noise blast + low body thump."""
    n = int(decay * SR)
    blast = lowpass(noise(n, seed), cutoff) * env(n, 0.0004, decay, 3.0)
    snap = highpass(noise(n, seed + 1), 5000.0) * env(n, 0.0001, decay * 0.18, 5.0) * crack
    thump = sweep(n, body, body * 0.45) * env(n, 0.001, decay * 0.6, 3.5) * 0.8
    return normalize(blast + snap + thump)


def impact(cutoff=1200.0, decay=0.12, seed=2):
    n = int(decay * SR)
    return normalize(lowpass(noise(n, seed), cutoff) * env(n, 0.0005, decay, 4.0))


def footstep(cutoff=900.0, decay=0.09, seed=3):
    n = int(decay * SR)
    scuff = lowpass(noise(n, seed), cutoff) * env(n, 0.002, decay, 3.0)
    return normalize(scuff * 0.7)


def explosion(decay=1.6, seed=4):
    n = int(decay * SR)
    rumble = sweep(n, 110.0, 24.0) * env(n, 0.004, decay, 1.8)
    debris = lowpass(noise(n, seed), 2200.0) * env(n, 0.001, decay, 2.2)
    return normalize(rumble * 1.2 + debris * 0.8)


def ui(freq=880.0, decay=0.11):
    n = int(decay * SR)
    t = np.arange(n) / SR
    tone = np.sin(2 * np.pi * freq * t) + 0.28 * np.sin(2 * np.pi * freq * 2 * t)
    return normalize(tone * env(n, 0.003, decay, 3.0))


def pickup(f0=520.0, f1=1400.0, decay=0.22):
    n = int(decay * SR)
    return normalize(sweep(n, f0, f1) * env(n, 0.004, decay, 2.0))


def whoosh(decay=0.45, seed=5):
    n = int(decay * SR)
    band = lowpass(highpass(noise(n, seed), 400.0), 2600.0)
    swell = np.sin(np.linspace(0.0, np.pi, n)) ** 1.5
    return normalize(band * swell)


VOICES = {
    "gunshot": gunshot, "impact": impact, "footstep": footstep,
    "explosion": explosion, "ui": ui, "pickup": pickup, "whoosh": whoosh,
}


# ── io ──────────────────────────────────────────────────────────────────────

def write_wav(path: Path, samples: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = np.clip(samples, -1.0, 1.0)
    data = (pcm * 32767.0).astype("<i2").tobytes()
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data)


def main(argv: list) -> int:
    if len(argv) < 4:
        print(__doc__, file=sys.stderr)
        return 2

    game_dir, asset_id, kind = argv[1], argv[2], argv[3]
    if kind not in VOICES:
        print(f"unknown kind '{kind}'. one of: {', '.join(sorted(VOICES))}", file=sys.stderr)
        return 2

    params = {}
    for arg in argv[4:]:
        k, _, v = arg.partition("=")
        params[k.lstrip("-")] = float(v)

    samples = VOICES[kind](**params)

    base = REPO_ROOT / game_dir
    dest = base / "assets" / "generated" / f"{asset_id}.wav"
    write_wav(dest, samples)
    print(f"==> wrote {dest} ({len(samples) / SR:.2f}s)")

    # Record in the manifest. Import lazily so audiogen works standalone.
    sys.path.insert(0, str(REPO_ROOT / "tools" / "artgen"))
    from manifest import load, save, sha256  # noqa: E402

    mpath = base / "assets" / "manifest.json"
    data = load(mpath)
    entry = {
        "id": asset_id,
        "kind": "audio",
        "tool": "audiogen",
        "script": "tools/audiogen/synth.py",
        "args": [kind] + [f"{k}={v}" for k, v in params.items()],
        "path": str(dest.relative_to(base)),
        "sha256": sha256(dest),
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
    }
    data["assets"] = sorted(
        [a for a in data["assets"] if a["id"] != asset_id] + [entry],
        key=lambda a: a["id"],
    )
    save(mpath, data)
    print(f"==> manifest updated: {asset_id} ({entry['sha256'][:12]})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
