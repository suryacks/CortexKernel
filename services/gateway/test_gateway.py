import unittest

from gateway import evaluate_request
from rate_limiter import RateLimiter


class TestEvaluateRequest(unittest.TestCase):
    def test_missing_api_key_is_rejected(self):
        limiter = RateLimiter(capacity=5, refill_rate=1)
        allowed, status, _ = evaluate_request(None, {"dev-key"}, limiter)
        self.assertFalse(allowed)
        self.assertEqual(status, 401)

    def test_unknown_api_key_is_rejected(self):
        limiter = RateLimiter(capacity=5, refill_rate=1)
        allowed, status, _ = evaluate_request("wrong-key", {"dev-key"}, limiter)
        self.assertFalse(allowed)
        self.assertEqual(status, 401)

    def test_valid_key_within_limit_is_allowed(self):
        limiter = RateLimiter(capacity=5, refill_rate=1)
        allowed, status, _ = evaluate_request("dev-key", {"dev-key"}, limiter)
        self.assertTrue(allowed)
        self.assertEqual(status, 200)

    def test_valid_key_over_limit_is_rate_limited(self):
        limiter = RateLimiter(capacity=1, refill_rate=0.001)
        evaluate_request("dev-key", {"dev-key"}, limiter)
        allowed, status, _ = evaluate_request("dev-key", {"dev-key"}, limiter)
        self.assertFalse(allowed)
        self.assertEqual(status, 429)

    def test_different_keys_have_independent_limits(self):
        limiter = RateLimiter(capacity=1, refill_rate=0.001)
        evaluate_request("dev-key", {"dev-key", "other-key"}, limiter)
        allowed, status, _ = evaluate_request("other-key", {"dev-key", "other-key"}, limiter)
        self.assertTrue(allowed)
        self.assertEqual(status, 200)


if __name__ == "__main__":
    unittest.main()
