import json
from http.server import BaseHTTPRequestHandler, HTTPServer

from bandit import Contradiction, EpsilonGreedyRanker
from persistence import load_state, save_state

STATE_PATH = "bandit_state.json"

ranker = EpsilonGreedyRanker(epsilon=0.1)
load_state(ranker, STATE_PATH)


class RankingHandler(BaseHTTPRequestHandler):
    def _send_json(self, status: int, payload) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self) -> None:
        length = int(self.headers.get("Content-Length", 0))
        raw_body = self.rfile.read(length) if length > 0 else b"{}"
        try:
            payload = json.loads(raw_body)
        except json.JSONDecodeError:
            self._send_json(400, {"error": "invalid JSON body"})
            return

        if self.path == "/rank":
            self._handle_rank(payload)
        elif self.path == "/feedback":
            self._handle_feedback(payload)
        else:
            self._send_json(404, {"error": f"unknown path: {self.path}"})

    def _handle_rank(self, payload) -> None:
        try:
            contradictions = [
                Contradiction(id=c["id"], category=c["category"], confidence=float(c["confidence"]))
                for c in payload.get("contradictions", [])
            ]
        except (KeyError, TypeError, ValueError) as exc:
            self._send_json(400, {"error": f"malformed contradiction: {exc}"})
            return

        ranked = ranker.rank(contradictions)
        self._send_json(200, {
            "ranked": [{"id": c.id, "category": c.category, "confidence": c.confidence} for c in ranked]
        })

    def _handle_feedback(self, payload) -> None:
        try:
            category = payload["category"]
            reward = float(payload["reward"])
        except (KeyError, TypeError, ValueError) as exc:
            self._send_json(400, {"error": f"malformed feedback: {exc}"})
            return

        ranker.record_feedback(category, reward)
        save_state(ranker, STATE_PATH)
        self._send_json(200, {"value_estimates": ranker.value_estimates()})

    def log_message(self, format: str, *args) -> None:
        pass


def run(port: int = 8081) -> None:
    server = HTTPServer(("0.0.0.0", port), RankingHandler)
    server.serve_forever()


if __name__ == "__main__":
    run()
