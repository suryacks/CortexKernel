import unittest

from rate_limiter import RateLimiter


class TestRateLimiter(unittest.TestCase):
    def test_invalid_capacity_raises(self):
        with self.assertRaises(ValueError):
            RateLimiter(capacity=0, refill_rate=1)

    def test_invalid_refill_rate_raises(self):
        with self.assertRaises(ValueError):
            RateLimiter(capacity=1, refill_rate=0)

    def test_allows_up_to_capacity_then_blocks(self):
        limiter = RateLimiter(capacity=2, refill_rate=1)
        self.assertTrue(limiter.allow("k", now=0.0))
        self.assertTrue(limiter.allow("k", now=0.0))
        self.assertFalse(limiter.allow("k", now=0.0))

    def test_refills_over_time(self):
        limiter = RateLimiter(capacity=2, refill_rate=1)
        limiter.allow("k", now=0.0)
        limiter.allow("k", now=0.0)
        self.assertFalse(limiter.allow("k", now=0.1))
        self.assertTrue(limiter.allow("k", now=1.0))

    def test_keys_are_independent(self):
        limiter = RateLimiter(capacity=1, refill_rate=1)
        self.assertTrue(limiter.allow("a", now=0.0))
        self.assertTrue(limiter.allow("b", now=0.0))
        self.assertFalse(limiter.allow("a", now=0.0))


if __name__ == "__main__":
    unittest.main()
