"""Command line interface.

Usage:
    python -m amy_training_nnue stats FILE...
    python -m amy_training_nnue inspect FILE [--offset N] [--count N]
    python -m amy_training_nnue train FILE... [--epochs N] [--batch-size N] ...
    python -m amy_training_nnue export CHECKPOINT [--out amy.net]
    python -m amy_training_nnue evaluate NETWORK FILE... [--count N]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import mlx.core as mx
import numpy as np

from .bpd import read_records, write_records
from .features import N_FEATURES
from .model import NnueNet
from .quantize import (
    NETWORK_QUANT_SIZE,
    QuantizedNet,
    quantize_parameters,
    read_network_quant,
    write_network_quant,
)
from .train import TrainConfig, _float_metrics, _fmt_metrics, _quant_metrics, train


def cmd_stats(args) -> int:
    total = 0
    invalid = 0
    scores: list[int] = []
    turns: list[int] = []
    for path in args.files:
        with open(path, "rb") as f:
            size = f.seek(0, 2)
        n = 0
        for record in read_records(path):
            n += 1
            if record.validate():
                invalid += 1
            scores.append(record.score)
            turns.append(record.turn)
        total += n
        print(f"{path}: {n} records ({size} bytes)")
    if not scores:
        print("no records")
        return 1
    s = np.asarray(scores)
    print(f"total: {total} records, {invalid} invalid")
    print(
        f"turn: white {sum(t == 0 for t in turns)}, "
        f"black {sum(t == 1 for t in turns)}"
    )
    print(
        f"score (millipawns): min {s.min()}, max {s.max()}, "
        f"mean {s.mean():.0f}, median {np.median(s):.0f}, "
        f"std {s.std():.0f}"
    )
    return 0


def cmd_inspect(args) -> int:
    from .position import position_from_record

    shown = 0
    for i, record in enumerate(read_records(args.file)):
        if i < args.offset:
            continue
        problems = record.validate()
        pos = position_from_record(record)
        print(
            f"[{i}] score {record.score}  {'INVALID: ' + '; '.join(problems) if problems else pos.epd()}"
        )
        shown += 1
        if shown >= args.count:
            break
    return 0


def cmd_train(args) -> int:
    config = TrainConfig(
        paths=args.files,
        output_dir=args.output,
        epochs=args.epochs,
        batch_size=args.batch_size,
        lr=args.lr,
        weight_decay=args.weight_decay,
        weight_bound=args.weight_bound if args.weight_bound > 0 else None,
        val_fraction=args.val_fraction,
        seed=args.seed,
        target_scale=args.target_scale,
        log_every=args.log_every,
        save_every=args.save_every,
        resume=args.resume,
    )
    train(config)
    return 0


def cmd_export(args) -> int:
    model = NnueNet.load(args.checkpoint)
    params = {k: np.array(v) for k, v in model.parameters.items()}
    quantized = quantize_parameters(params)
    n = write_network_quant(args.out, quantized)
    assert n == NETWORK_QUANT_SIZE
    print(f"wrote {args.out} ({n} bytes)")
    return 0


def cmd_evaluate(args) -> int:
    qnet = QuantizedNet.from_file(args.network)
    from .dataset import BpdDataset

    ds = BpdDataset(args.files)
    idx = np.arange(ds.n)
    if args.count:
        idx = idx[: args.count]
    qm = _quant_metrics(qnet, ds, idx)
    fm = (
        _float_metrics(NnueNet.load(args.checkpoint), ds, idx)
        if args.checkpoint
        else None
    )
    print(f"positions: {len(idx)}")
    print(f"quantized: {_fmt_metrics(qm)}")
    if fm:
        print(f"float    : {_fmt_metrics(fm)}")
    # Show a few positions with both scores.
    from .bpd import read_records
    from .position import position_from_record

    records = []
    for path in args.files:
        records.extend(list(read_records(path)))
    preds = qnet.evaluate(
        ds.feat_w,
        ds.feat_b,
        ds.pfeat_w,
        ds.pfeat_b,
        ds.afeat_w,
        ds.afeat_b,
        ds.turn,
        ds.phase_index,
    )
    shown = 0
    for i in range(ds.n):
        print(
            f"nn {preds[i]:6d} mp  stored {ds.score[i]:6d} mp  "
            f"{position_from_record(records[i]).epd()}"
        )
        shown += 1
        if shown >= args.show:
            break
    return 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        prog="amy-training-nnue", description=__doc__.strip().splitlines()[0]
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("stats", help="show statistics of BPD files")
    p.add_argument("files", nargs="+")
    p.set_defaults(func=cmd_stats)

    p = sub.add_parser("inspect", help="show individual positions as EPD")
    p.add_argument("file")
    p.add_argument("--offset", type=int, default=0)
    p.add_argument("--count", type=int, default=5)
    p.set_defaults(func=cmd_inspect)

    p = sub.add_parser("train", help="train the network")
    p.add_argument("files", nargs="+")
    p.add_argument("--output", default="checkpoints")
    p.add_argument("--epochs", type=int, default=30)
    p.add_argument("--batch-size", type=int, default=256)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--weight-decay", type=float, default=0.0)
    p.add_argument(
        "--weight-bound",
        type=float,
        default=1.9,
        help="keep weight tensors (not biases) within [-B, B] each step; 0 disables",
    )
    p.add_argument("--val-fraction", type=float, default=0.1)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--target-scale", type=float, default=10000.0)
    p.add_argument("--log-every", type=int, default=20)
    p.add_argument(
        "--save-every",
        type=int,
        default=10,
        help="save a checkpoint every N epochs (and at the end)",
    )
    p.add_argument("--resume", default=None, help="checkpoint dir to resume from")
    p.set_defaults(func=cmd_train)

    p = sub.add_parser("export", help="quantize a checkpoint to amy.net")
    p.add_argument("checkpoint")
    p.add_argument("--out", default="amy.net")
    p.set_defaults(func=cmd_export)

    p = sub.add_parser("evaluate", help="evaluate a quantized network on BPD data")
    p.add_argument("network")
    p.add_argument("files", nargs="+")
    p.add_argument("--count", type=int, default=0)
    p.add_argument("--checkpoint", default=None, help="float checkpoint for comparison")
    p.add_argument("--show", type=int, default=3, help="positions to show")
    p.set_defaults(func=cmd_evaluate)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
