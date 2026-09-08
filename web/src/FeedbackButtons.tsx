interface Props {
  category: string
  onFeedback: (category: string, reward: number) => void
}

export default function FeedbackButtons({ category, onFeedback }: Props) {
  return (
    <span style={{ marginLeft: 8 }}>
      <button onClick={() => onFeedback(category, 1.0)}>Useful</button>
      <button onClick={() => onFeedback(category, 0.0)} style={{ marginLeft: 4 }}>
        Dismiss
      </button>
    </span>
  )
}
