#include"graph.h"
#include <vector>
#include <climits>
#include <deque>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;


void PrintDV(const vector<vector<int>> &v) {
    for (int i = 1; i < v.size(); i++) {
        cout << "from: " << i << endl;
        for (int j = 1; j < v.size(); j++) {
            cout << " to: " << j << " cost: " << v[i][j] << endl;
        }
    }
}

void PrintEveryNodeDV(Graph *g) {
    for (auto i : g->nodes) {
        PrintDV(i.second->dv);
    }
}

string PrintTopoEntry(Node *node) {
    string res = "";
    for (int i = 1; i < node->dv.size(); i++) {
        if (node->dv[node->num][i] == INT_MAX) continue;
        res += to_string(i) + " " + to_string(node->next_hop[node->num][i]) + " " + to_string(node->dv[node->num][i]) + "\n";
        
        // cout << i << " " << node->next_hop[node->num][i] << " " << node->dv[node->num][i] << endl;
    }
    return res;
}


string PrintFWDTable(Graph *g) {
    string res = "";
    for (int i = 1; i <= g->nodes.size(); i++) {
        res += PrintTopoEntry(g->nodes[i]);
    }
    return res;
}

void SendVecToNeighbours(Node *n, unordered_set<int> &s) {
    for (auto neighbour : n->neighbours) {
        neighbour.second->dv[n->num] = n->dv[n->num];
        neighbour.second->next_hop[n->num] = n->next_hop[n->num];
        s.insert(neighbour.first);
    }
}

void DistanceVectorInit(Graph *g, unordered_set<int> &s) {
    for (auto node : g->nodes) {
        cout << 1 << endl;
        for (auto neighbour : node.second->neighbours_cost) {
            cout << "neigbour: " << neighbour.first << "node: " << node.first << " size: " << node.second->dv[node.first].size() << endl;
            cout << node.second->dv[node.first].size() << endl;
            cout << g->nodes.size() << endl;
            for (int i : node.second->dv[node.first]) {
                cout << "i am " << i << endl;
            }



            node.second->dv[node.first][neighbour.first] = neighbour.second;
            cout << "passed" << endl;
            node.second->next_hop[node.first][neighbour.first] = neighbour.first;
        }
        cout << 2 << endl;
        node.second->dv[node.first][node.first] = 0;
        node.second->next_hop[node.first][node.first] = node.first;
        cout << 3 << endl;
        SendVecToNeighbours(node.second, s);
        cout << 4 << endl;
    }
}


unordered_set<int> DistanceVectorConvergeHelper(Graph *g, unordered_set<int> &s) {
    unordered_set<int> new_notified;
    cout << "im in" << endl;
    for (auto i : s) {
        Node *cur_node = g->nodes[i];
        for (int j = 1; j < cur_node->dv[cur_node->num].size(); j++) {
            int min_distance = cur_node->dv[cur_node->num][j];
            int next = cur_node->next_hop[cur_node->num][j];
            for (auto neighbour : cur_node->neighbours) {

                int tmp_dis = cur_node->neighbours_cost[neighbour.first] + neighbour.second->dv[neighbour.first][j];

                // if (cur_node->num == 1 && j == 3) {
                //     cout << "from 1 to 3: neighbour: " << neighbour.first << " cur->neig: " << cur_node->dv[cur_node->num][neighbour.first] << " neigh->j: "<< neighbour.second->dv[neighbour.first][j] << endl;
                // }
                // if (cur_node->num == 1 && j == 3) {
                //     cout << "from 1 to 3: cost is: " << tmp_dis << " hop: " << neighbour.first << " prev next: " << next << " edge cost: "<< cur_node->neighbours_cost[neighbour.first] << endl << endl;
                // }
                if (tmp_dis < 0) continue;
                if (tmp_dis < min_distance || (tmp_dis == min_distance && neighbour.first < next)) {

                    next = neighbour.first;
                    min_distance = tmp_dis;
                }
            }
            if ((cur_node->dv[cur_node->num][j] > min_distance) || 
                ((cur_node->dv[cur_node->num][j] == min_distance) && (cur_node->next_hop[cur_node->num][j] > next))) {
                    
                cur_node->dv[cur_node->num][j] = min_distance;
                cur_node->next_hop[cur_node->num][j] = next;
                SendVecToNeighbours(cur_node, new_notified);
            }
        }
    }
    return new_notified;
}

void DistanceVectorAlgo(Graph *g) {
    unordered_set<int> notified_list;
    DistanceVectorInit(g, notified_list);
    cout << "inited" << endl;
    while (!notified_list.empty()) {
        notified_list = DistanceVectorConvergeHelper(g, notified_list);
    }
}

string SendMsg(Graph *g, int from_node, int to_node, string msg) {
    string res = "from " + to_string(from_node) + " to " + to_string(to_node) + " cost ";
    int cost = g->nodes[from_node]->dv[from_node][to_node];
    if (cost == INT_MAX) {
        res += "infinite hops unreachable ";
    } else {
        res += to_string(cost) + " hops ";
        int cur_node = from_node;
        do {
            res += to_string(cur_node) + " ";
            cur_node = g->nodes[cur_node]->next_hop[cur_node][to_node];
        } while (cur_node != to_node);
    }
    res += msg;
    return res;
}


string ReadAndSendMessage(string file_name, Graph *g) {
    string res = "";
    string line;
    ifstream fd;
    fd.open(file_name);
    if (fd.is_open()) {
        while (getline(fd, line)) {
            stringstream ss{line};
            int from_node = 0;
            int to_node = 0;
            string msg = "message ";
            ss >> from_node >> to_node;
            if (from_node == 0) continue;
            //cout << "start "<< from_node << " " << to_node << "\n";
            msg += line.substr(to_string(from_node).size() + to_string(to_node).size() + 2);
            //cout << "end\n";
            res += SendMsg(g, from_node, to_node, msg) + "\n";
        }
        fd.close();
    }
    return res;
}

string ReadChanges(string file_name, vector<tuple<int, int, int>> &data) {
    string res = "";
    string line;
    ifstream fd;
    fd.open(file_name);
    if (fd.is_open()) {
        while (getline(fd, line)) {
            cout << "what1?\n";
            stringstream ss{line};
            int from_node = 0;
            int to_node = 0;
            int cost = 0;
            ss >> from_node >> to_node >> cost;
            Graph *new_graph = new Graph();
            tuple<int, int, int> tup1 = make_tuple(from_node, to_node, cost);
            bool changed = false;
            bool isDelete = cost == -999;
            cout << "what2?\n";
            if (isDelete) {
                int pos = 0;
                for (int i = 0; i < data.size(); i++) {
                    if (get<0>(data[i]) == from_node && get<1>(data[i]) == to_node) {
                        pos = i;
                        break;
                    }
                }
                data.erase(data.begin() + pos);
                changed = true;
                cout << "what3?\n";
            } else {
                for (auto &t : data) {
                    if (get<0>(t) == from_node && get<1>(t) == to_node) {
                        get<2>(t) = cost;
                        changed = true;
                        break;
                    }
                }
                 cout << "what4?\n";
            }

            if (!changed) {
               data.emplace_back(tup1);
            }


            new_graph->data = data;
            cout << "what5?\n";
            BuildGraph(new_graph);
            cout << "what6?\n";
            DistanceVectorAlgo(new_graph);
            cout << "what7?\n";
            res += PrintFWDTable(new_graph);
            cout << "what8?\n";
            res += ReadAndSendMessage("messagefile", new_graph);
            cout << "what9?\n";
        }
        fd.close();
    }
    return res;
}


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    ofstream fout;
    fout.open("output.txt", ostream::trunc);
    Graph *graph = new Graph();
    vector<tuple<int, int, int>> original_data = ReadData(argv[1]);
    graph->data = original_data;
    BuildGraph(graph);
    DistanceVectorAlgo(graph);
    cout << "bug1?\n";
    fout << PrintFWDTable(graph);
    cout << "bug2?\n";
    fout << ReadAndSendMessage(argv[2], graph);
    cout << "bug3?\n";
    fout << ReadChanges(argv[3], original_data);

    fout.close();
    return 0;
}

