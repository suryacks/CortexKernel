from enum import Enum
from typing import List

from pydantic import BaseModel, ConfigDict, Field


class NodeType(str, Enum):
    SELF = "Self"
    PERSON = "Person"
    PLACE = "Place"
    ORG = "Org"
    PROJECT = "Project"
    CONCEPT = "Concept"
    VALUE = "Value"
    EVENT = "Event"
    EMOTION = "Emotion"
    ARTIFACT = "Artifact"
    TASK = "Task"


class EdgeClass(str, Enum):
    STATED_VALUE = "StatedValue"
    BEHAVIOR_EVIDENCE = "BehaviorEvidence"
    FACT = "Fact"
    EMOTION = "Emotion"
    REFLECTION = "Reflection"


class NodeCandidate(BaseModel):
    id: str
    type: NodeType
    name: str


class EdgeCandidate(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    id: str
    from_id: str = Field(alias="from")
    predicate: str
    to_id: str = Field(alias="to")
    edge_class: EdgeClass


class ExtractionResult(BaseModel):
    nodes: List[NodeCandidate] = []
    edges: List[EdgeCandidate] = []


class ExtractRequest(BaseModel):
    text: str
    source_ref: str = ""


class FeedbackRequest(BaseModel):
    category: str
    reward: float
