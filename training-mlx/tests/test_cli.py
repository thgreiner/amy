"""End-to-end CLI tests."""

import struct
import subprocess
import sys
from datetime import date, datetime
from pathlib import Path

import pytest

from amy_training_nnue.quantize import HEADER_SIZE, NETWORK_QUANT_SIZE

DATA = Path(__file__).parent / "data" / "bpd_test.bin"


def run_cli(*args):
    return subprocess.run(
        [sys.executable, "-m", "amy_training_nnue", *args],
        capture_output=True,
        text=True,
        check=True,
    )


def test_stats(tmp_path):
    result = run_cli("stats", str(DATA))
    assert "5 records" in result.stdout
    assert "white 3" in result.stdout
    assert "black 2" in result.stdout
    assert "min 1, max 5" in result.stdout


def test_inspect(tmp_path):
    result = run_cli("inspect", str(DATA), "--count", "2", "--offset", "2")
    assert "[2]" in result.stdout
    assert "[3]" in result.stdout
    # record 2 is the position after 1.e4 e5 2.Nf3
    assert "5N2/PPPP1PPP/RNBQKB1R" in result.stdout
    assert "[1]" not in result.stdout


def test_train_and_export_and_evaluate(tmp_path):
    out = tmp_path / "ckpts"
    result = run_cli(
        "train",
        str(DATA),
        "--output",
        str(out),
        "--epochs",
        "2",
        "--batch-size",
        "2",
        "--seed",
        "0",
    )
    assert "loaded 5 positions" in result.stdout
    final = out / "final"
    assert (final / "weights.npz").exists()
    assert (final / "meta.json").exists()
    quant = out / "amy.net"
    assert quant.exists()
    assert quant.stat().st_size == NETWORK_QUANT_SIZE

    result = run_cli("evaluate", str(quant), str(DATA), "--show", "2")
    assert "positions: 5" in result.stdout
    assert "quantized:" in result.stdout

    # export from a checkpoint produces the same payload; name and date
    # are per-export metadata (random codename, ISO8601 timestamp now)
    result = run_cli("export", str(final), "--out", str(tmp_path / "nq2.quant"))
    b1, b2 = quant.read_bytes(), (tmp_path / "nq2.quant").read_bytes()
    assert b1[HEADER_SIZE:] == b2[HEADER_SIZE:]
    for b in (b1, b2):
        name, date_ = struct.unpack_from("<IHHH2xIHH8BH32s32s2x", b, 0)[-2:]
        assert name.rstrip(b"\x00") != b""
        t = datetime.fromisoformat(date_.rstrip(b"\x00").decode("ascii"))
        assert t.date() == date.today()


def test_inspect_invalid_position(tmp_path):
    """A file with a corrupt record must be flagged, not crash."""
    from amy_training_nnue.bpd import WHITE, BpdRecord, read_records

    records = list(read_records(DATA))
    bad = BpdRecord((0, 0, 0, 0), 0, WHITE)  # no kings
    path = tmp_path / "bad.bpd"
    with open(path, "wb") as f:
        f.write(bad.encode())
    result = run_cli("inspect", str(path))
    assert "INVALID" in result.stdout
    assert "expected exactly one white king" in result.stdout
    assert "expected exactly one black king" in result.stdout
