import os
from typing import Any, Dict, List

import httpx

RANKING_URL = os.environ.get("RANKING_URL", "http://localhost:8081")


def contradiction_to_rank_item(contradiction: Dict[str, Any]) -> Dict[str, Any]:
    edge_a = contradiction["edge_a"]
    edge_b = contradiction["edge_b"]
    return {
        "id": f"{edge_a['id']}:{edge_b['id']}",
        "category": contradiction["type"],
        "confidence": float(edge_a.get("confidence", 1.0)),
    }


def rank_contradictions(contradictions: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    if not contradictions:
        return []

    items = [contradiction_to_rank_item(c) for c in contradictions]
    response = httpx.post(f"{RANKING_URL}/rank", json={"contradictions": items}, timeout=10.0)
    response.raise_for_status()
    ranked_ids = [item["id"] for item in response.json()["ranked"]]

    by_id = {contradiction_to_rank_item(c)["id"]: c for c in contradictions}
    return [by_id[rid] for rid in ranked_ids if rid in by_id]


def submit_feedback(category: str, reward: float) -> Dict[str, Any]:
    response = httpx.post(f"{RANKING_URL}/feedback", json={"category": category, "reward": reward}, timeout=10.0)
    response.raise_for_status()
    return response.json()
