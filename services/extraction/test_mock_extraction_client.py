import unittest

from mock_extraction_client import MockExtractionClient
from schemas import EdgeClass


class TestMockExtractionClient(unittest.TestCase):
    def setUp(self):
        self.client = MockExtractionClient()

    def test_extracts_stated_value_edge(self):
        result = self.client.extract("I value my health.")
        self.assertTrue(any(
            e.edge_class == EdgeClass.STATED_VALUE and e.to_id == "my-health" for e in result.edges
        ))

    def test_extracts_behavior_evidence_edge(self):
        result = self.client.extract("I worked on the project all night.")
        self.assertTrue(any(e.edge_class == EdgeClass.BEHAVIOR_EVIDENCE for e in result.edges))

    def test_always_includes_self_node(self):
        result = self.client.extract("Nothing matches here.")
        self.assertTrue(any(n.id == "self" for n in result.nodes))

    def test_no_duplicate_object_nodes_for_repeated_phrase(self):
        result = self.client.extract("I value my health. I value my health.")
        health_nodes = [n for n in result.nodes if n.id == "my-health"]
        self.assertEqual(len(health_nodes), 1)

    def test_edges_reference_self_as_subject(self):
        result = self.client.extract("I value honesty.")
        self.assertTrue(all(e.from_id == "self" for e in result.edges))

    def test_no_match_produces_no_edges(self):
        result = self.client.extract("The weather is nice today.")
        self.assertEqual(result.edges, [])


if __name__ == "__main__":
    unittest.main()
