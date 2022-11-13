#ifndef __GRAPH_H__
#define __GRAPH_H__


#include <string>
#include <unordered_map>
#include <deque>
#include <vector>
#include <tuple>

class Graph;

class Node {
  public:
    int num;
    int num_of_nodes = 0;
    std::vector<std::vector<int>> dv;
    std::vector<std::vector<int>> next_hop;
    std::unordered_map<int, Node *> neighbours;
    std::unordered_map<int, int> neighbours_cost;
    Node(int n) : num{n} {};
    ~Node(){};
    void update(int neighbour, Node *n, int cost);
};


class Graph {
  public:
    std::unordered_map<int, Node *> nodes;
    std::vector<std::tuple<int, int, int>> data;
    Graph(){};
    ~Graph(){};
    bool contains(int n);
    void add(int node);
    Node *get(int node);
};

void BuildGraph(Graph *graph);
std::vector<std::tuple<int, int, int>> ReadData(const std::string &file_name);
#endif