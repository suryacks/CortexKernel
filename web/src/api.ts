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

async function getJson<T>(path: string): Promise<T> {
  const response = await fetch(`${API_BASE}${path}`)
  if (!response.ok) {
    throw new Error(`${path} failed with status ${response.status}`)
  }
  return response.json() as Promise<T>
}

export function fetchNodes(): Promise<NodeDto[]> {
  return getJson<NodeDto[]>('/nodes')
}

export function fetchEdges(): Promise<EdgeDto[]> {
  return getJson<EdgeDto[]>('/edges')
}

export function fetchContradictions(): Promise<Contradiction[]> {
  return getJson<Contradiction[]>('/contradictions')
}
