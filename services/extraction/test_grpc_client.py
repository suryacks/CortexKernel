import unittest
from concurrent import futures

import grpc

import cortexkernel_pb2
import cortexkernel_pb2_grpc
from grpc_client import StorageGrpcClient
from schemas import EdgeCandidate, NodeCandidate, NodeType


class FakeServicer(cortexkernel_pb2_grpc.CortexKernelStorageServicer):
    def __init__(self):
        self.nodes = {}

    def AddNode(self, request, context):
        self.nodes[request.id] = request
        return cortexkernel_pb2.NodeReply(found=True, id=request.id, type=request.type, name=request.name)

    def AddEdge(self, request, context):
        return cortexkernel_pb2.EdgeReply(
            id=request.id,
            **{"from": getattr(request, "from")},
            predicate=request.predicate,
            to=request.to,
            edge_class=request.edge_class,
        )

    def GetNode(self, request, context):
        node = self.nodes.get(request.id)
        if node is None:
            return cortexkernel_pb2.NodeReply(found=False)
        return cortexkernel_pb2.NodeReply(found=True, id=node.id, type=node.type, name=node.name)


class TestStorageGrpcClient(unittest.TestCase):
    def setUp(self):
        self.server = grpc.server(futures.ThreadPoolExecutor(max_workers=2))
        self.servicer = FakeServicer()
        cortexkernel_pb2_grpc.add_CortexKernelStorageServicer_to_server(self.servicer, self.server)
        port = self.server.add_insecure_port("127.0.0.1:0")
        self.server.start()
        self.client = StorageGrpcClient(f"127.0.0.1:{port}")

    def tearDown(self):
        self.client.close()
        self.server.stop(None)

    def test_add_node_then_get_node_round_trips(self):
        self.client.add_node(NodeCandidate(id="n1", type=NodeType.PERSON, name="Test"))
        reply = self.client.get_node("n1")
        self.assertTrue(reply.found)
        self.assertEqual(reply.name, "Test")

    def test_get_node_missing_returns_not_found(self):
        reply = self.client.get_node("does-not-exist")
        self.assertFalse(reply.found)

    def test_add_edge_round_trips_from_field(self):
        edge = EdgeCandidate.model_validate({"id": "e1", "from": "a", "predicate": "p", "to": "b", "edge_class": "Fact"})
        reply = self.client.add_edge(edge)
        self.assertEqual(reply.id, "e1")
        self.assertEqual(getattr(reply, "from"), "a")

    def test_store_all_adds_nodes_and_edges(self):
        nodes = [NodeCandidate(id="n1", type=NodeType.CONCEPT, name="x")]
        edges = [EdgeCandidate.model_validate({"id": "e1", "from": "n1", "predicate": "p", "to": "n1", "edge_class": "Fact"})]
        self.client.store_all(nodes, edges)
        reply = self.client.get_node("n1")
        self.assertTrue(reply.found)


if __name__ == "__main__":
    unittest.main()
