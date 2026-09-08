/// <reference types="vite/client" />

export interface NodeDto {
  id: string
  type: string
  name: string
}

export interface EdgeDto {
  id: string
  from: string
  to: string
  predicate: string
  edge_class: string
  invalid_at: string | null
}

export interface Contradiction {
  type: string
  subject_id: string
  predicate: string
  edge_a: EdgeDto
  edge_b: EdgeDto
}

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:8080'
const EXTRACTION_API_BASE = import.meta.env.VITE_EXTRACTION_API_URL ?? 'http://localhost:8082'

async function getJson<T>(base: string, path: string): Promise<T> {
  const response = await fetch(`${base}${path}`)
  if (!response.ok) {
    throw new Error(`${path} failed with status ${response.status}`)
  }
  return response.json() as Promise<T>
}

export function fetchNodes(): Promise<NodeDto[]> {
  return getJson<NodeDto[]>(API_BASE, '/nodes')
}

export function fetchEdges(): Promise<EdgeDto[]> {
  return getJson<EdgeDto[]>(API_BASE, '/edges')
}

export function fetchContradictions(): Promise<Contradiction[]> {
  return getJson<Contradiction[]>(API_BASE, '/contradictions')
}

export async function fetchRankedContradictions(): Promise<Contradiction[]> {
  const response = await fetch(`${EXTRACTION_API_BASE}/contradictions/ranked`)
  if (!response.ok) {
    throw new Error(`/contradictions/ranked failed with status ${response.status}`)
  }
  const body = (await response.json()) as { ranked: Contradiction[] }
  return body.ranked
}

export async function submitFeedback(category: string, reward: number): Promise<void> {
  const response = await fetch(`${EXTRACTION_API_BASE}/feedback`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ category, reward }),
  })
  if (!response.ok) {
    throw new Error(`/feedback failed with status ${response.status}`)
  }
}
