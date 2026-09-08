import type { Contradiction } from './api'
import FeedbackButtons from './FeedbackButtons'

interface Props {
  contradictions: Contradiction[]
  onFeedback: (category: string, reward: number) => void
}

export default function ContradictionsPanel({ contradictions, onFeedback }: Props) {
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
          <FeedbackButtons category={c.type} onFeedback={onFeedback} />
        </li>
      ))}
    </ul>
  )
}
