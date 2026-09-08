import type { Contradiction } from './api'

interface Props {
  contradictions: Contradiction[]
}

export default function ContradictionsPanel({ contradictions }: Props) {
  if (contradictions.length === 0) {
    return <p>No contradictions detected.</p>
  }

  return (
    <ul>
      {contradictions.map((c) => (
        <li key={`${c.edge_a.id}-${c.edge_b.id}`}>
          <strong>{c.type === 'direct' ? 'Direct contradiction' : 'Value/behavior mismatch'}</strong>
          {': '}
          {c.subject_id} --{c.predicate}--&gt; {c.edge_a.to} vs {c.edge_b.to}
        </li>
      ))}
    </ul>
  )
}
