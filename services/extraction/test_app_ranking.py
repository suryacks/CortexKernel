import unittest
from unittest.mock import MagicMock, patch

from fastapi.testclient import TestClient

import app


class TestContradictionsRankedEndpoint(unittest.TestCase):
    def setUp(self):
        self.client = TestClient(app.app)

    @patch("app.rank_contradictions")
    @patch("app.httpx.Client")
    def test_contradictions_ranked_calls_storage_then_ranking_client(self, mock_client_cls, mock_rank):
        contradiction = {
            "type": "direct",
            "subject_id": "self",
            "predicate": "lives_in",
            "edge_a": {"id": "e1"},
            "edge_b": {"id": "e2"},
        }
        mock_rank.return_value = [contradiction]

        mock_response = MagicMock()
        mock_response.json.return_value = [contradiction]
        mock_response.raise_for_status.return_value = None

        mock_client_instance = MagicMock()
        mock_client_instance.get.return_value = mock_response
        mock_client_instance.__enter__.return_value = mock_client_instance
        mock_client_instance.__exit__.return_value = False
        mock_client_cls.return_value = mock_client_instance

        response = self.client.get("/contradictions/ranked")

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json(), {"ranked": [contradiction]})
        mock_rank.assert_called_once_with([contradiction])

    @patch("app.submit_feedback")
    def test_feedback_forwards_to_ranking_client(self, mock_submit_feedback):
        mock_submit_feedback.return_value = {"value_estimates": {"direct": 1.0}}

        response = self.client.post("/feedback", json={"category": "direct", "reward": 1.0})

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json(), {"value_estimates": {"direct": 1.0}})
        mock_submit_feedback.assert_called_once_with("direct", 1.0)

    def test_feedback_missing_reward_is_422(self):
        response = self.client.post("/feedback", json={"category": "direct"})
        self.assertEqual(response.status_code, 422)


if __name__ == "__main__":
    unittest.main()
