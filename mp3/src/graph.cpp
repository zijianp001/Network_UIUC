#include "graph.h"
#include <fstream>
#include <sstream>
#include <climits>
#include <vector>

using namespace std;

void Node::update(int neighbour, Node *n, int cost) {
    (this->neighbours_cost)[neighbour] = cost;
    (this->neighbours)[neighbour] = n;
}

bool Graph::contains(int node) {
    return (this->nodes).count(node) == 1;
}

Node *Graph::get(int node) {
    return this->nodes[node];
}

void Graph::add(int node) {
    this->nodes[node] = new Node(node);
}

vector<tuple<int, int, int>> ReadData(const string &file_name) {
    vector<tuple<int, int, int>> res;
    string line;
    ifstream fd;
    fd.open(file_name);
    if (fd.is_open()) {
        while (getline(fd, line)) {
            stringstream ss{line};
            int from_node = 0;
            int to_node = 0;
            int cost = 0;
            ss >> from_node >> to_node >> cost;
            res.emplace_back(make_tuple(from_node, to_node, cost));
        }
        fd.close();
    }
    return res;
}



void BuildGraph(Graph *graph) {
    for (auto i : graph->data) {
        int from_node = get<0>(i);
        int to_node = get<1>(i);
        int cost = get<2>(i);
        if (!graph->contains(from_node)) {
            graph->add(from_node);
        }
        if (!graph->contains(to_node)) {
            graph->add(to_node);
        }
        graph->get(from_node)->update(to_node, graph->get(to_node), cost);
        graph->get(to_node)->update(from_node, graph->get(from_node), cost);
    }
    for (auto i : graph->nodes) {
        i.second->dv = vector<vector<int>>(graph->nodes.size() + 1, 
                                           vector<int>(graph->nodes.size() + 1, INT_MAX));
        i.second->next_hop = vector<vector<int>>(graph->nodes.size() + 1, 
                                                 vector<int>(graph->nodes.size() + 1, INT_MAX));
    }
}

