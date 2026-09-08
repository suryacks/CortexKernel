import unittest

from ranking_client import contradiction_to_rank_item, rank_contradictions


class TestRankingClient(unittest.TestCase):
    def test_contradiction_to_rank_item_builds_correct_shape(self):
        contradiction = {
            "type": "direct",
            "edge_a": {"id": "e1", "confidence": 0.8},
            "edge_b": {"id": "e2", "confidence": 0.9},
        }
        item = contradiction_to_rank_item(contradiction)
        self.assertEqual(item["id"], "e1:e2")
        self.assertEqual(item["category"], "direct")
        self.assertEqual(item["confidence"], 0.8)

    def test_contradiction_to_rank_item_defaults_confidence(self):
        contradiction = {
            "type": "value_behavior",
            "edge_a": {"id": "e1"},
            "edge_b": {"id": "e2"},
        }
        item = contradiction_to_rank_item(contradiction)
        self.assertEqual(item["confidence"], 1.0)

    def test_rank_contradictions_returns_empty_for_empty_input(self):
        self.assertEqual(rank_contradictions([]), [])


if __name__ == "__main__":
    unittest.main()
