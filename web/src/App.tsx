import { useEffect, useState } from 'react'
import { fetchContradictions, fetchEdges, fetchNodes } from './api'
import type { Contradiction, EdgeDto, NodeDto } from './api'
import GraphView from './GraphView'
import ContradictionsPanel from './ContradictionsPanel'

export default function App() {
  const [nodes, setNodes] = useState<NodeDto[]>([])
  const [edges, setEdges] = useState<EdgeDto[]>([])
  const [contradictions, setContradictions] = useState<Contradiction[]>([])
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    Promise.all([fetchNodes(), fetchEdges(), fetchContradictions()])
      .then(([n, e, c]) => {
        setNodes(n)
        setEdges(e)
        setContradictions(c)
      })
      .catch((err) => setError(String(err)))
  }, [])

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
      <h2>Detected contradictions</h2>
      <ContradictionsPanel contradictions={contradictions} />
    </div>
  )
}
