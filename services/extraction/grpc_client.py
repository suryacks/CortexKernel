from typing import Iterable

import grpc

import cortexkernel_pb2
import cortexkernel_pb2_grpc
from schemas import EdgeCandidate, NodeCandidate


class StorageGrpcClient:
    def __init__(self, target: str = "localhost:50051") -> None:
        self._channel = grpc.insecure_channel(target)
        self._stub = cortexkernel_pb2_grpc.CortexKernelStorageStub(self._channel)

    def add_node(self, node: NodeCandidate) -> cortexkernel_pb2.NodeReply:
        request = cortexkernel_pb2.NodeRequest(id=node.id, type=node.type.value, name=node.name)
        return self._stub.AddNode(request)

    def add_edge(self, edge: EdgeCandidate) -> cortexkernel_pb2.EdgeReply:
        request = cortexkernel_pb2.EdgeRequest(
            id=edge.id,
            **{"from": edge.from_id},
            predicate=edge.predicate,
            to=edge.to_id,
            edge_class=edge.edge_class.value,
        )
        return self._stub.AddEdge(request)

    def get_node(self, node_id: str) -> cortexkernel_pb2.NodeReply:
        request = cortexkernel_pb2.GetNodeRequest(id=node_id)
        return self._stub.GetNode(request)

    def store_all(self, nodes: Iterable[NodeCandidate], edges: Iterable[EdgeCandidate]) -> None:
        for node in nodes:
            self.add_node(node)
        for edge in edges:
            self.add_edge(edge)

    def close(self) -> None:
        self._channel.close()
