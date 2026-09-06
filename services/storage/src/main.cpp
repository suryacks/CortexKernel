#include "../include/graph_store.hpp"
#include "../include/httplib.h"

#include <iostream>
#include <sstream>

int main() {
    kg::GraphStore store; 
    httplib::Server svr; 
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });

    svr.Post("/nodes", [&store](const httplib::Request& req, httplib::Response& res) {
        kg::Node n; 
        n.id = req.get_param_value("id");
        n.name = req.get_param_value("name");
        n.type = kg::NodeType::Concept;
        n.layer = "life";

        store.add_node(n);
        std::ostringstream out;
        out << "Node added. Total Nodes: " << store.node_count();
        res.set_content(out.str(), "text/plain");
    });

    svr.Get("/nodes/count", [&store](const httplib::Request&, httplib::Response& res) {
        std::ostringstream out;
        out  << store.node_count();
        res.set_content(out.str(), "text/plain");
    });

    std::cout << "CortexKernel storage service listening on port 8080..." << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}