import unittest

from bandit import Contradiction, EpsilonGreedyRanker


class TestEpsilonGreedyRanker(unittest.TestCase):
    def test_invalid_epsilon_raises(self):
        with self.assertRaises(ValueError):
            EpsilonGreedyRanker(epsilon=1.5)

    def test_rank_empty_list_returns_empty(self):
        ranker = EpsilonGreedyRanker(epsilon=0.0, seed=1)
        self.assertEqual(ranker.rank([]), [])

    def test_rank_falls_back_to_confidence_before_feedback(self):
        ranker = EpsilonGreedyRanker(epsilon=0.0, seed=1)
        low = Contradiction("low", "direct", 0.2)
        high = Contradiction("high", "direct", 0.9)

        ranked = ranker.rank([low, high])

        self.assertEqual([c.id for c in ranked], ["high", "low"])

    def test_feedback_changes_future_ranking(self):
        ranker = EpsilonGreedyRanker(epsilon=0.0, seed=1)
        direct = Contradiction("d", "direct", 0.5)
        mismatch = Contradiction("m", "value_behavior", 0.5)

        ranker.record_feedback("value_behavior", 1.0)
        ranker.record_feedback("direct", 0.0)
        ranked = ranker.rank([direct, mismatch])

        self.assertEqual([c.id for c in ranked], ["m", "d"])

    def test_value_estimates_track_incremental_average(self):
        ranker = EpsilonGreedyRanker(epsilon=0.0, seed=1)

        ranker.record_feedback("direct", 1.0)
        ranker.record_feedback("direct", 0.0)

        self.assertAlmostEqual(ranker.value_estimates()["direct"], 0.5)

    def test_explore_branch_still_returns_every_contradiction(self):
        ranker = EpsilonGreedyRanker(epsilon=1.0, seed=1)
        contradictions = [Contradiction("a", "direct", 0.1), Contradiction("b", "value_behavior", 0.9)]

        ranked = ranker.rank(contradictions)

        self.assertEqual({c.id for c in ranked}, {"a", "b"})


if __name__ == "__main__":
    unittest.main()
