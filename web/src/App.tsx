import { useCallback, useEffect, useState } from 'react'
import { fetchEdges, fetchNodes, fetchRankedContradictions, submitFeedback } from './api'
import type { Contradiction, EdgeDto, NodeDto } from './api'
import GraphView from './GraphView'
import ContradictionsPanel from './ContradictionsPanel'

export default function App() {
  const [nodes, setNodes] = useState<NodeDto[]>([])
  const [edges, setEdges] = useState<EdgeDto[]>([])
  const [contradictions, setContradictions] = useState<Contradiction[]>([])
  const [error, setError] = useState<string | null>(null)

  const loadContradictions = useCallback(() => {
    fetchRankedContradictions()
      .then(setContradictions)
      .catch((err) => setError(String(err)))
  }, [])

  useEffect(() => {
    Promise.all([fetchNodes(), fetchEdges()])
      .then(([n, e]) => {
        setNodes(n)
        setEdges(e)
      })
      .catch((err) => setError(String(err)))
    loadContradictions()
  }, [loadContradictions])

  const handleFeedback = (category: string, reward: number) => {
    submitFeedback(category, reward)
      .then(loadContradictions)
      .catch((err) => setError(String(err)))
  }

  const contradictedNodeIds = new Set<string>()
  for (const c of contradictions) {
    contradictedNodeIds.add(c.edge_a.to)
    contradictedNodeIds.add(c.edge_b.to)
  }

  return (
    <div style={{ fontFamily: 'sans-serif', padding: 16 }}>
      <h1>CortexKernel</h1>
      {error && <p style={{ color: 'red' }}>{error}</p>}
      <GraphView nodes={nodes} edges={edges} contradictedNodeIds={contradictedNodeIds} />
      <h2>Detected contradictions (ranked)</h2>
      <ContradictionsPanel contradictions={contradictions} onFeedback={handleFeedback} />
    </div>
  )
}
