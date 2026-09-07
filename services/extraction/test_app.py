import unittest

from fastapi.testclient import TestClient

from app import app


class TestExtractionApp(unittest.TestCase):
    def setUp(self):
        self.client = TestClient(app)

    def test_health(self):
        response = self.client.get("/health")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json(), {"status": "ok"})

    def test_extract_returns_nodes_and_edges(self):
        response = self.client.post("/extract", json={"text": "I value my health.", "source_ref": "test"})
        self.assertEqual(response.status_code, 200)
        body = response.json()
        self.assertTrue(any(n["id"] == "self" for n in body["nodes"]))
        self.assertTrue(any(e["edge_class"] == "StatedValue" for e in body["edges"]))

    def test_extract_missing_text_field_is_422(self):
        response = self.client.post("/extract", json={})
        self.assertEqual(response.status_code, 422)


if __name__ == "__main__":
    unittest.main()
