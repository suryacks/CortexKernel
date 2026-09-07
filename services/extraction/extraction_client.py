import os
from abc import ABC, abstractmethod
from typing import Optional

import httpx

from schemas import ExtractionResult


class ExtractionClient(ABC):
    @abstractmethod
    def extract(self, text: str, source_ref: str = "") -> ExtractionResult:
        raise NotImplementedError


class AnthropicExtractionClient(ExtractionClient):
    def __init__(self, api_key: Optional[str] = None, model: str = "claude-sonnet-5") -> None:
        self.api_key = api_key or os.environ.get("ANTHROPIC_API_KEY")
        if not self.api_key:
            raise RuntimeError("ANTHROPIC_API_KEY is required for AnthropicExtractionClient")
        self.model = model

    def extract(self, text: str, source_ref: str = "") -> ExtractionResult:
        response = httpx.post(
            "https://api.anthropic.com/v1/messages",
            headers={
                "x-api-key": self.api_key,
                "anthropic-version": "2023-06-01",
                "content-type": "application/json",
            },
            json={
                "model": self.model,
                "max_tokens": 1024,
                "messages": [{"role": "user", "content": self._build_prompt(text, source_ref)}],
            },
            timeout=30.0,
        )
        response.raise_for_status()
        content = response.json()["content"][0]["text"]
        return ExtractionResult.model_validate_json(content)

    @staticmethod
    def _build_prompt(text: str, source_ref: str) -> str:
        return (
            "Extract knowledge-graph Node and Edge candidates from the text below. "
            "Respond with ONLY a JSON object matching this shape: "
            '{"nodes": [{"id": str, "type": one of '
            "Self/Person/Place/Org/Project/Concept/Value/Event/Emotion/Artifact/Task, "
            '"name": str}], "edges": [{"id": str, "from": str, "predicate": str, "to": str, '
            '"edge_class": one of StatedValue/BehaviorEvidence/Fact/Emotion/Reflection}]}. '
            "Distinguish StatedValue (what the person says they believe/want) from "
            "BehaviorEvidence (what they actually did). "
            f"source_ref: {source_ref}\n\nText:\n{text}"
        )
