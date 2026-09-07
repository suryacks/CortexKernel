import os

import httpx
from fastapi import FastAPI, HTTPException

from extraction_client import AnthropicExtractionClient, ExtractionClient
from mock_extraction_client import MockExtractionClient
from schemas import ExtractRequest, ExtractionResult

STORAGE_URL = os.environ.get("STORAGE_URL", "http://localhost:8080")


def build_client() -> ExtractionClient:
    backend = os.environ.get("EXTRACTION_BACKEND", "mock")
    if backend == "anthropic":
        return AnthropicExtractionClient()
    return MockExtractionClient()


app = FastAPI(title="CortexKernel Extraction Service")
client = build_client()


@app.get("/health")
def health():
    return {"status": "ok"}


@app.post("/extract", response_model=ExtractionResult)
def extract(request: ExtractRequest) -> ExtractionResult:
    return client.extract(request.text, request.source_ref)


@app.post("/extract-and-store")
def extract_and_store(request: ExtractRequest):
    result = client.extract(request.text, request.source_ref)
    stored_nodes = []
    stored_edges = []

    with httpx.Client(timeout=10.0) as http:
        for node in result.nodes:
            response = http.post(f"{STORAGE_URL}/nodes", json=node.model_dump(mode="json"))
            if response.status_code not in (200, 201):
                raise HTTPException(status_code=502, detail=f"storage rejected node {node.id}: {response.text}")
            stored_nodes.append(response.json())

        for edge in result.edges:
            response = http.post(f"{STORAGE_URL}/edges", json=edge.model_dump(mode="json", by_alias=True))
            if response.status_code not in (200, 201):
                raise HTTPException(status_code=502, detail=f"storage rejected edge {edge.id}: {response.text}")
            stored_edges.append(response.json())

    return {"nodes": stored_nodes, "edges": stored_edges}
