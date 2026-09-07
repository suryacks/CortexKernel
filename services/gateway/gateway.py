import json
import os
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Optional, Set, Tuple

from rate_limiter import RateLimiter

STORAGE_URL = os.environ.get("STORAGE_URL", "http://localhost:8080")
API_KEYS: Set[str] = set(os.environ.get("CORTEXKERNEL_API_KEYS", "dev-key").split(","))
RATE_LIMIT_CAPACITY = float(os.environ.get("RATE_LIMIT_CAPACITY", "20"))
RATE_LIMIT_REFILL_PER_SEC = float(os.environ.get("RATE_LIMIT_REFILL_PER_SEC", "5"))

limiter = RateLimiter(capacity=RATE_LIMIT_CAPACITY, refill_rate=RATE_LIMIT_REFILL_PER_SEC)


def evaluate_request(api_key: Optional[str], valid_keys: Set[str], rate_limiter: RateLimiter) -> Tuple[bool, int, str]:
    if not api_key or api_key not in valid_keys:
        return False, 401, "invalid or missing API key"
    if not rate_limiter.allow(api_key):
        return False, 429, "rate limit exceeded"
    return True, 200, "ok"


class GatewayHandler(BaseHTTPRequestHandler):
    def _send_json(self, status: int, payload: dict) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _relay(self, status: int, body: bytes) -> None:
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _proxy(self) -> None:
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length > 0 else None
        request = urllib.request.Request(
            STORAGE_URL + self.path,
            data=body,
            method=self.command,
            headers={"Content-Type": "application/json"} if body else {},
        )
        try:
            with urllib.request.urlopen(request) as response:
                self._relay(response.status, response.read())
        except urllib.error.HTTPError as exc:
            self._relay(exc.code, exc.read())
        except urllib.error.URLError as exc:
            self._send_json(502, {"error": f"upstream unreachable: {exc.reason}"})

    def _handle(self) -> None:
        api_key = self.headers.get("X-API-Key")
        allowed, status, reason = evaluate_request(api_key, API_KEYS, limiter)
        if not allowed:
            self._send_json(status, {"error": reason})
            return
        self._proxy()

    def do_GET(self) -> None:
        self._handle()

    def do_POST(self) -> None:
        self._handle()

    def log_message(self, format: str, *args) -> None:
        pass


def run(port: int = 8090) -> None:
    server = HTTPServer(("0.0.0.0", port), GatewayHandler)
    server.serve_forever()


if __name__ == "__main__":
    run()
