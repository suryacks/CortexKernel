import re
import uuid

from extraction_client import ExtractionClient
from schemas import EdgeCandidate, EdgeClass, ExtractionResult, NodeCandidate, NodeType

STATED_VALUE_PATTERNS = [
    re.compile(r"\bi (?:value|believe in|care about|prioritize)\s+(.+?)[\.\!\?]?$", re.IGNORECASE),
]
BEHAVIOR_PATTERNS = [
    re.compile(r"\bi (?:spent|worked on|did|stayed up for)\s+(.+?)[\.\!\?]?$", re.IGNORECASE),
]


def _slugify(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


class MockExtractionClient(ExtractionClient):
    def extract(self, text: str, source_ref: str = "") -> ExtractionResult:
        nodes = [NodeCandidate(id="self", type=NodeType.SELF, name="Self")]
        edges = []
        seen_objects = set()

        for sentence in re.split(r"(?<=[.!?])\s+", text.strip()):
            if not sentence:
                continue
            self._match(sentence, STATED_VALUE_PATTERNS, EdgeClass.STATED_VALUE, "values", nodes, edges, seen_objects)
            self._match(sentence, BEHAVIOR_PATTERNS, EdgeClass.BEHAVIOR_EVIDENCE, "did", nodes, edges, seen_objects)

        return ExtractionResult(nodes=nodes, edges=edges)

    @staticmethod
    def _match(sentence, patterns, edge_class, predicate, nodes, edges, seen_objects):
        for pattern in patterns:
            match = pattern.search(sentence)
            if not match:
                continue
            object_text = match.group(1).strip()
            object_id = _slugify(object_text)
            if not object_id:
                continue
            if object_id not in seen_objects:
                nodes.append(NodeCandidate(id=object_id, type=NodeType.CONCEPT, name=object_text))
                seen_objects.add(object_id)
            edges.append(EdgeCandidate.model_validate({
                "id": str(uuid.uuid4()),
                "from": "self",
                "to": object_id,
                "predicate": predicate,
                "edge_class": edge_class,
            }))
