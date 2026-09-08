import { useEffect, useRef } from 'react'
import * as d3 from 'd3'
import type { EdgeDto, NodeDto } from './api'

interface Props {
  nodes: NodeDto[]
  edges: EdgeDto[]
  contradictedNodeIds: Set<string>
}

interface SimNode extends d3.SimulationNodeDatum {
  id: string
  type: string
  name: string
}

interface SimLink {
  source: string
  target: string
  predicate: string
  edge_class: string
}

const WIDTH = 720
const HEIGHT = 480

export default function GraphView({ nodes, edges, contradictedNodeIds }: Props) {
  const svgRef = useRef<SVGSVGElement | null>(null)

  useEffect(() => {
    if (!svgRef.current) return

    const simNodes: SimNode[] = nodes.map((n) => ({ ...n }))
    const simLinks: SimLink[] = edges
      .filter((e) => !e.invalid_at)
      .map((e) => ({ source: e.from, target: e.to, predicate: e.predicate, edge_class: e.edge_class }))

    const svg = d3.select(svgRef.current)
    svg.selectAll('*').remove()

    const simulation = d3
      .forceSimulation(simNodes)
      .force(
        'link',
        d3
          .forceLink<SimNode, SimLink>(simLinks)
          .id((d) => d.id)
          .distance(90),
      )
      .force('charge', d3.forceManyBody().strength(-220))
      .force('center', d3.forceCenter(WIDTH / 2, HEIGHT / 2))

    const link = svg
      .append('g')
      .attr('stroke', '#999')
      .attr('stroke-opacity', 0.6)
      .selectAll('line')
      .data(simLinks)
      .join('line')
      .attr('stroke-width', 1.5)

    const node = svg
      .append('g')
      .selectAll<SVGCircleElement, SimNode>('circle')
      .data(simNodes)
      .join('circle')
      .attr('r', 10)
      .attr('fill', (d) => (contradictedNodeIds.has(d.id) ? '#e74c3c' : '#4f8ef7'))
      .call(
        d3
          .drag<SVGCircleElement, SimNode>()
          .on('start', (event, d) => {
            if (!event.active) simulation.alphaTarget(0.3).restart()
            d.fx = d.x
            d.fy = d.y
          })
          .on('drag', (event, d) => {
            d.fx = event.x
            d.fy = event.y
          })
          .on('end', (event, d) => {
            if (!event.active) simulation.alphaTarget(0)
            d.fx = null
            d.fy = null
          }),
      )

    node.append('title').text((d) => `${d.name} (${d.type})`)

    const label = svg
      .append('g')
      .selectAll('text')
      .data(simNodes)
      .join('text')
      .text((d) => d.name)
      .attr('font-size', 10)
      .attr('dx', 12)
      .attr('dy', 4)

    simulation.on('tick', () => {
      link
        .attr('x1', (d) => (d.source as unknown as SimNode).x ?? 0)
        .attr('y1', (d) => (d.source as unknown as SimNode).y ?? 0)
        .attr('x2', (d) => (d.target as unknown as SimNode).x ?? 0)
        .attr('y2', (d) => (d.target as unknown as SimNode).y ?? 0)

      node.attr('cx', (d) => d.x ?? 0).attr('cy', (d) => d.y ?? 0)
      label.attr('x', (d) => d.x ?? 0).attr('y', (d) => d.y ?? 0)
    })

    return () => {
      simulation.stop()
    }
  }, [nodes, edges, contradictedNodeIds])

  return <svg ref={svgRef} width={WIDTH} height={HEIGHT} style={{ border: '1px solid #ddd' }} />
}
