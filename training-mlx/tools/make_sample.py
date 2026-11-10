"""Create a fixed-size training sample from the growing selfplay log.

tests/data/selfplay_sample.bin is a *frozen* snapshot committed to git so
the training smoke test always runs on identical data. The selfplay log
(evaluation_log.bin in this directory) keeps growing with every selfplay
session, so tests must not read it directly.

To deliberately refresh the snapshot (e.g. when the selfplay distribution
has shifted a lot), run:

    python tools/make_sample.py [AMY_LOG]

and commit the result.

Defaults: AMY_LOG=evaluation_log.bin in the project root, 5% of records,
seed 42.
Indices are drawn without replacement and sorted, so the sample preserves
the log's original record order.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amy_training_nnue.bpd import read_records, write_records  # noqa: E402


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Sample the selfplay log into a fixed training fixture"
    )
    ap.add_argument(
        "log",
        nargs="?",
        default=str(Path(__file__).resolve().parents[1] / "evaluation_log.bin"),
    )
    ap.add_argument("--frac", type=float, default=0.05)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--out", default="tests/data/selfplay_sample.bin")
    args = ap.parse_args()

    records = list(read_records(args.log))
    n = len(records)
    k = max(1, min(n, int(n * args.frac)))
    rng = np.random.default_rng(args.seed)
    idx = np.sort(rng.choice(n, size=k, replace=False))
    write_records(args.out, [records[i] for i in idx])

    data = Path(args.out).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    print(f"source: {args.log} ({n:,} records)")
    print(
        f"sample: {args.out} ({k:,} records, {len(data):,} bytes, "
        f"frac {args.frac}, seed {args.seed})"
    )
    print(f"sha256 {digest}")


if __name__ == "__main__":
    main()
