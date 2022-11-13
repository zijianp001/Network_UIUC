#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<map>
#include<sstream>
#include<fstream>
#include<iostream>
#include<unordered_map>
#include<unordered_set>
#include<vector>
#include<climits>
#include<queue>
using namespace std;
unordered_map<int, unordered_map<int ,int>> edges;
unordered_set<int> nodes;
map<int, map<int, int>> next_hop;
map<int, map<int, int>> distance_tbl;
unordered_map<int, unordered_map<int, int>> P;

void readTopology(char* filename) {
	ifstream file;
	file.open(filename);
	int node1;
	int node2;
	int cost;
	while(file >> node1 >> node2 >> cost) {
		edges[node1][node2] = cost;
		edges[node2][node1] = cost;
		nodes.insert(node1);
		nodes.insert(node2);

	}
	file.close();
}

void distanceInitial() {
        distance_tbl.clear();
	next_hop.clear();
	P.clear();
        for(int source : nodes) {
                unordered_map <int, int> neighbors = edges[source];
                for(int target : nodes) {
                        if(source == target) {
                                distance_tbl[source][target] = 0;
                        }
                        else if(neighbors.count(target) > 0) {
                                distance_tbl[source][target] = neighbors[target];
				P[source][target] = source;
                        }
                        else {
                                distance_tbl[source][target] = INT_MAX;
                        }
                }
        }
}


void dijkstra(int source) {
	unordered_set<int> N;
	N.insert(source);
	unordered_map<int, int> neighbors = edges[source];
	map<int, int> spanning_tree;

	int start = 0;
	bool end = false;
	while(end == false) {
		end = true;
		int next_node = -1;
		int cost_to_next_node = INT_MAX;
		for(int neighbor : nodes) {
			if(N.count(neighbor) <= 0) {
				int cost_to_this_node = distance_tbl[source][neighbor];
				if(cost_to_this_node < cost_to_next_node) {
					next_node = neighbor;
					cost_to_next_node = cost_to_this_node;
				}
			}
		}
		if(next_node != -1) {
			end = false;
			N.insert(next_node);
			spanning_tree[start] = next_node;
			start++;
			//Update cost for all adjacent nodes of next_node that not in N
			unordered_map<int, int> neighbors_of_next_node = edges[next_node];	
			for(auto i = neighbors_of_next_node.begin(); i != neighbors_of_next_node.end(); i++) {
				int neighbor_of_next_node = i -> first;
				int cost_to_neighbor = i -> second;
				if(distance_tbl[source][next_node] + cost_to_neighbor < distance_tbl[source][neighbor_of_next_node]) {
					distance_tbl[source][neighbor_of_next_node] = distance_tbl[source][next_node] + cost_to_neighbor;
					P[source][neighbor_of_next_node] = next_node;
				}
				else if(distance_tbl[source][next_node] + cost_to_neighbor == distance_tbl[source][neighbor_of_next_node] && next_node < P[source][neighbor_of_next_node]) {
					P[source][neighbor_of_next_node] = next_node;
				}
			}
		}
	}
	for(int i = 0; i < spanning_tree.size(); i++) {
		int temp = spanning_tree[i];
		if(P[source][temp] == source) {
			next_hop[source][temp] = temp;
		}
		else {
			next_hop[source][temp] = next_hop[source][P[source][temp]];
		}
	}
	next_hop[source][source] = source;
	distance_tbl[source][source] = 0;

}

void computeForwardTable() {
	distanceInitial();
	for(int source : nodes) {
		dijkstra(source);
	}
}

void printForwardTable() {
	ofstream outfile;
	outfile.open("output.txt", ios_base::app);
	for(auto i = distance_tbl.begin(); i != distance_tbl.end(); i++) {
		map<int, int> temp = i -> second;
		for(auto j = temp.begin(); j != temp.end(); j++) {
			if(j -> second < INT_MAX) {
				int source = i -> first;
				int target = j -> first;
				int cost = j -> second;
				int next = next_hop[source][target];
				outfile << target << " " << next << " " << cost << "\n";

			}
		}
	}
	outfile.close();
}


void printSingleMessage(int source, int target) {
        ofstream outfile;
        outfile.open("output.txt", ios_base::app);
        if(distance_tbl.count(source) <= 0 || distance_tbl[source].count(target) <= 0 || distance_tbl[source][target] == INT_MAX) {
                outfile << "from " << source << " to " << target << " cost infinite hops unreachable message ";
        }
        else if(source == target) {
                int cost = 0;
                outfile << "from " << source << " to " << target << " cost " << cost << " hops " << source << " message ";
        }
        else {
                int next_op = next_hop[source][target];
                int cost = distance_tbl[source][target];
                queue<int> path;
                int curr = source;
                while(curr != target) {
                        path.push(curr);
                        curr = next_op;
                        next_op = next_hop[next_op][target];
                }
                outfile << "from " << source << " to " << target << " cost " << cost << " hops ";
                while(!path.empty()) {
                        int next_op = path.front();
                        path.pop();
                        outfile << next_op << " ";
                }
                outfile << "message ";
        }
        outfile.close();
}


void printFullMessage(char* filename) {
	string line;
	ifstream message(filename);
	if(message.is_open()) {
		int s;
		int t;
		while(getline(message, line)) {
			stringstream ss;
			ss.str(line);
			ss >> s >> t;
			int split1 = line.find(" ");
			string post_split1 = line.substr(split1 + 1);
			int split2 = post_split1.find(" ");
			string post_split2 = post_split1.substr(split2 + 1);
			printSingleMessage(s, t);
			ofstream out;
			out.open("output.txt", ios_base::app);
			out << post_split2 << "\n";
			out.close();
		}
		message.close();
	}
}


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fclose(fpOut);

    readTopology(argv[1]);
    
    //for(auto i = edges.begin(); i != edges.end(); i++) {
	    //cout << i->first;
	    //for(auto j = i->second.begin(); j != i->second.end(); j++) {
		    //cout << "Connected to: ";
		    //cout << j -> first;
		    //cout << "Cost: ";
		    //cout << j -> second;
		    //cout << "\n";
	    //}
	    //cout << "\n\n\n";
    //}
    computeForwardTable();
    printForwardTable();
    ifstream file;
    int node1;
    int node2;
    int cost;
    printFullMessage(argv[2]);
    file.open(argv[3]);

    while(file >> node1 >> node2 >> cost) {
	    ofstream outfile_change_num;
	    outfile_change_num.open("output.txt", ios_base::app);
	    //outfile_change_num << "---At this point, " << change_num << " change is applied" << "\n";
	    outfile_change_num.close();
	    if(cost == -999) {
		    edges[node1].erase(node2);
		    edges[node2].erase(node1);
	    }
	    else {
		    edges[node1][node2] = cost;
		    edges[node2][node1] = cost;
	    }
	    computeForwardTable();
	    printForwardTable();
	    printFullMessage(argv[2]);
    }
    file.close();


    return 0;
}

