#!/usr/bin/env bash
set -euo pipefail

GATEWAY_URL="http://localhost:8090"
EXTRACTION_URL="http://localhost:8082"
API_KEY="dev-key"

echo "=== bringing up the stack (docker compose up -d) ==="
docker compose up -d
echo

echo "=== waiting for the gateway to answer ==="
for _ in $(seq 1 30); do
  if curl -s -o /dev/null -H "X-API-Key: ${API_KEY}" "${GATEWAY_URL}/health"; then
    break
  fi
  sleep 1
done
curl -s -H "X-API-Key: ${API_KEY}" "${GATEWAY_URL}/health"
echo
echo

echo "=== extracting a journal entry with a stated-value/behavior gap ==="
curl -s -H "Content-Type: application/json" -X POST "${EXTRACTION_URL}/extract-and-store" \
  -d '{"text":"I value my health. I worked on the project all night.","source_ref":"demo"}'
echo
echo

echo "=== posting a direct factual contradiction (lives_in: nyc vs la) ==="
curl -s -H "X-API-Key: ${API_KEY}" -X POST "${GATEWAY_URL}/nodes" -d '{"id":"self","type":"Self","name":"Self"}' > /dev/null
curl -s -H "X-API-Key: ${API_KEY}" -X POST "${GATEWAY_URL}/edges" -d '{"id":"demo-e1","from":"self","predicate":"lives_in","to":"nyc","edge_class":"Fact"}' > /dev/null
curl -s -H "X-API-Key: ${API_KEY}" -X POST "${GATEWAY_URL}/edges" -d '{"id":"demo-e2","from":"self","predicate":"lives_in","to":"la","edge_class":"Fact"}' > /dev/null
echo "done"
echo

echo "=== posting a same-predicate value/behavior mismatch (prioritizes: work vs rest) ==="
curl -s -H "X-API-Key: ${API_KEY}" -X POST "${GATEWAY_URL}/edges" -d '{"id":"demo-e3","from":"self","predicate":"prioritizes","to":"work","edge_class":"StatedValue"}' > /dev/null
curl -s -H "X-API-Key: ${API_KEY}" -X POST "${GATEWAY_URL}/edges" -d '{"id":"demo-e4","from":"self","predicate":"prioritizes","to":"rest","edge_class":"BehaviorEvidence"}' > /dev/null
echo "done (the mock extractor above used different predicates for values/did, so it didn't"
echo "trigger the exact-match detector by itself - this pair does, on purpose, so the ranked"
echo "list below actually has two categories to reorder)"
echo

echo "=== ranked contradictions, before any feedback ==="
curl -s "${EXTRACTION_URL}/contradictions/ranked"
echo
echo

echo "=== submitting feedback: value/behavior mismatches are useful, direct ones are not ==="
echo "(deliberately favoring whichever category ISN'T already ranked first, so the reorder is visible below)"
curl -s -H "Content-Type: application/json" -X POST "${EXTRACTION_URL}/feedback" -d '{"category":"value_behavior","reward":1.0}'
echo
curl -s -H "Content-Type: application/json" -X POST "${EXTRACTION_URL}/feedback" -d '{"category":"direct","reward":0.0}'
echo
echo

echo "=== ranked contradictions again, after feedback (order should reflect it) ==="
echo "(the bandit is epsilon-greedy and explores randomly ~10% of the time by design -"
echo "if the order looks unchanged, that's the explore branch, not a bug - rerun to see it settle)"
curl -s "${EXTRACTION_URL}/contradictions/ranked"
echo
echo

echo "=== metrics + dashboards ==="
echo "Prometheus: http://localhost:9090"
echo "Grafana:    http://localhost:3000"
echo
echo "Services are still running. Stop them with: docker compose down"
