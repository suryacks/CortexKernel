import json
from pathlib import Path
from typing import Union

from bandit import EpsilonGreedyRanker


def save_state(ranker: EpsilonGreedyRanker, path: Union[str, Path]) -> None:
    with open(path, "w") as f:
        json.dump(ranker.export_state(), f)


def load_state(ranker: EpsilonGreedyRanker, path: Union[str, Path]) -> bool:
    state_path = Path(path)
    if not state_path.exists():
        return False
    with open(state_path) as f:
        state = json.load(f)
    ranker.load_state(state)
    return True
