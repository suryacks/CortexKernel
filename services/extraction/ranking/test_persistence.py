import os
import tempfile
import unittest

from bandit import Contradiction, EpsilonGreedyRanker
from persistence import load_state, save_state


class TestPersistence(unittest.TestCase):
    def test_load_state_returns_false_when_file_missing(self):
        ranker = EpsilonGreedyRanker(epsilon=0.0, seed=1)
        self.assertFalse(load_state(ranker, "/tmp/cortexkernel-does-not-exist.json"))

    def test_save_then_load_round_trips_value_estimates(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = os.path.join(tmp_dir, "bandit_state.json")

            original = EpsilonGreedyRanker(epsilon=0.0, seed=1)
            original.record_feedback("direct", 1.0)
            original.record_feedback("direct", 0.0)
            original.record_feedback("value_behavior", 1.0)
            save_state(original, path)

            restored = EpsilonGreedyRanker(epsilon=0.0, seed=1)
            self.assertTrue(load_state(restored, path))

            self.assertEqual(restored.value_estimates(), original.value_estimates())

    def test_restored_ranker_produces_same_ranking_as_original(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = os.path.join(tmp_dir, "bandit_state.json")

            original = EpsilonGreedyRanker(epsilon=0.0, seed=1)
            original.record_feedback("value_behavior", 1.0)
            original.record_feedback("direct", 0.0)
            save_state(original, path)

            restored = EpsilonGreedyRanker(epsilon=0.0, seed=1)
            load_state(restored, path)

            contradictions = [Contradiction("d", "direct", 0.5), Contradiction("m", "value_behavior", 0.5)]
            self.assertEqual(
                [c.id for c in restored.rank(contradictions)],
                [c.id for c in original.rank(contradictions)],
            )


if __name__ == "__main__":
    unittest.main()
