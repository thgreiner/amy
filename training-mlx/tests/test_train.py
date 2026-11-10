"""Tests for the training loop."""

import json
from pathlib import Path

import numpy as np
import pytest

pytest.importorskip("mlx.core")

from amy_training_nnue.model import WEIGHT_NAMES, NnueNet  # noqa: E402
from amy_training_nnue.quantize import NETWORK_QUANT_SIZE  # noqa: E402
from amy_training_nnue.train import TrainConfig, train  # noqa: E402

DATA = Path(__file__).parent / "data" / "bpd_test.bin"
SAMPLE = Path(__file__).parent / "data" / "selfplay_sample.bin"


def test_train_keeps_weights_in_bounds(tmp_path):
    """With a deliberately huge lr the weights are pushed into the bound;
    they must still end up inside [-1, 1] and the export must succeed."""
    config = TrainConfig(
        paths=[str(DATA)],
        output_dir=str(tmp_path / "ckpts"),
        epochs=3,
        batch_size=5,
        lr=1.0,
        weight_decay=0.0,
        seed=0,
        log_every=1,
        save_every=10,
    )
    final = train(config)
    model = NnueNet.load(final)
    for name in WEIGHT_NAMES:
        w = np.array(getattr(model, name))
        assert np.all((w >= -1.0) & (w <= 1.0)), name
    quant = Path(config.output_dir) / "amy.net"
    assert quant.stat().st_size == NETWORK_QUANT_SIZE


def test_train_smoke_on_sample(tmp_path):
    """One epoch on the frozen 5% selfplay sample: the pipeline must
    actually learn (beat the zero-prediction baseline — rmse of the
    tanh-squeezed targets, about 0.23 = 2300 mp on this sample — and
    correlate with them) and export a valid network."""
    config = TrainConfig(
        paths=[str(SAMPLE)],
        output_dir=str(tmp_path / "ckpts"),
        epochs=1,
        batch_size=512,
        lr=1e-3,
        seed=0,
        log_every=0,
    )
    final = train(config)
    meta = json.loads((final / "meta.json").read_text())
    val = meta["val"]
    # Zero-prediction baseline in tanh space is about 0.23 (2300 mp).
    assert val["rmse_mp"] < 2300, val
    assert val["corr"] > 0.5, val
    assert (final / "weights.npz").exists()
    quant = Path(config.output_dir) / "amy.net"
    assert quant.stat().st_size == NETWORK_QUANT_SIZE
