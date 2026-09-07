import time
from typing import Dict, Optional


class TokenBucket:
    def __init__(self, capacity: float, refill_rate: float) -> None:
        self.capacity = capacity
        self.refill_rate = refill_rate
        self.tokens = capacity
        self.last_refill: Optional[float] = None

    def _refill(self, now: float) -> None:
        if self.last_refill is None:
            self.last_refill = now
            return
        elapsed = now - self.last_refill
        self.tokens = min(self.capacity, self.tokens + elapsed * self.refill_rate)
        self.last_refill = now

    def try_consume(self, amount: float = 1.0, now: Optional[float] = None) -> bool:
        now = time.monotonic() if now is None else now
        self._refill(now)
        if self.tokens >= amount:
            self.tokens -= amount
            return True
        return False


class RateLimiter:
    def __init__(self, capacity: float, refill_rate: float) -> None:
        if capacity <= 0 or refill_rate <= 0:
            raise ValueError("capacity and refill_rate must be positive")
        self.capacity = capacity
        self.refill_rate = refill_rate
        self._buckets: Dict[str, TokenBucket] = {}

    def _bucket(self, key: str) -> TokenBucket:
        return self._buckets.setdefault(key, TokenBucket(self.capacity, self.refill_rate))

    def allow(self, key: str, now: Optional[float] = None) -> bool:
        return self._bucket(key).try_consume(now=now)
