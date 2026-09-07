import random
from dataclasses import dataclass
from typing import Dict, List, Optional


@dataclass
class Contradiction:
    id: str
    category: str
    confidence: float


@dataclass
class ArmStats:
    pulls: int = 0
    value_estimate: float = 0.0

    def update(self, reward: float) -> None:
        self.pulls += 1
        self.value_estimate += (reward - self.value_estimate) / self.pulls


class EpsilonGreedyRanker:
    def __init__(self, epsilon: float = 0.1, seed: Optional[int] = None) -> None:
        if not 0.0 <= epsilon <= 1.0:
            raise ValueError("epsilon must be between 0 and 1")
        self.epsilon = epsilon
        self._arms: Dict[str, ArmStats] = {}
        self._rng = random.Random(seed)

    def _arm(self, category: str) -> ArmStats:
        return self._arms.setdefault(category, ArmStats())

    def record_feedback(self, category: str, reward: float) -> None:
        self._arm(category).update(reward)

    def rank(self, contradictions: List[Contradiction]) -> List[Contradiction]:
        if not contradictions:
            return []

        categories = sorted({c.category for c in contradictions})
        for category in categories:
            self._arm(category)

        if self._rng.random() < self.epsilon:
            explore_category = self._rng.choice(categories)
            return sorted(
                contradictions,
                key=lambda c: (c.category != explore_category, -c.confidence),
            )

        return sorted(
            contradictions,
            key=lambda c: (-self._arm(c.category).value_estimate, -c.confidence),
        )

    def value_estimates(self) -> Dict[str, float]:
        return {category: stats.value_estimate for category, stats in self._arms.items()}
