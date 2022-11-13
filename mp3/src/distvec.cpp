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
map<int, map<int, int>> distance_vec;
map<int, map<int, int>> next_hop;



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
	//cout << "Size" << nodes.size() << "\n";
}

void distanceInitial() {
	distance_vec.clear();
	next_hop.clear();
	for(int source : nodes) {
		unordered_map <int, int> neighbors = edges[source];
		for(int target : nodes) {
			if(source == target) {
				distance_vec[source][target] = 0;
				next_hop[source][target] = target;
			}
			else if(neighbors.count(target) > 0) {
				distance_vec[source][target] = neighbors[target];
				next_hop[source][target] = target;
			}
			else {
				distance_vec[source][target] = INT_MAX;
			}
		}
	}
}

void computeDistance() {
	bool converge = false;
	while(converge == false) {
		converge = true;
		//Here we send from source to all its neighbors or the nodes currently connected, and up date the neighbor/connected nodes distance
		for(int source : nodes) {
			for(int neighbor : nodes) {
				//If this node is not currently connect to source, we do not sent distance vector message to this node
				//if(distance_vec[source][neighbor] == INT_MAX) {
				//	continue;
				//}
				if(edges.count(source) <= 0 || edges[source].count(neighbor) <= 0) {
					continue;
				}
				for(int source_vec : nodes) {
					if(distance_vec[source][source_vec] == INT_MAX || source == source_vec) {
						continue;
					}
					if(distance_vec[source][neighbor] + distance_vec[source][source_vec] < distance_vec[neighbor][source_vec]) {
						//if(neighbor == 1 && source_vec == 3) {
						//	cout << "Source:" << source << "  Neighbor:" << neighbor << "   Source_vec" << source_vec << "\n";
						//}
						distance_vec[neighbor][source_vec] = distance_vec[source][neighbor] + distance_vec[source][source_vec];
						next_hop[neighbor][source_vec] = source;
						converge = false;
					}
				}
			}
		}
	}
}

void printForwardTable() {
        ofstream outfile;
        outfile.open("output.txt", ios_base::app);
        for(auto i = distance_vec.begin(); i != distance_vec.end(); i++) {
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
        if(distance_vec.count(source) <= 0 || distance_vec[source].count(target) <= 0 || distance_vec[source][target] == INT_MAX) {
                outfile << "from " << source << " to " << target << " cost infinite hops unreachable message ";
        }
        else if(source == target) {
                int cost = 0;
                outfile << "from " << source << " to " << target << " cost " << cost << " hops " << source << " message ";
        }
        else {
                int next_op = next_hop[source][target];
                int cost = distance_vec[source][target];
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
        //outfile << "a new source and target";
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
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }


    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fclose(fpOut);

    readTopology(argv[1]);
    distanceInitial();
    computeDistance();
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
            outfile_change_num.close();
            if(cost == -999) {
                    edges[node1].erase(node2);
                    edges[node2].erase(node1);
            }
            else {
                    edges[node1][node2] = cost;
                    edges[node2][node1] = cost;
            }
            distanceInitial();
            computeDistance();
	    printForwardTable();
            printFullMessage(argv[2]);
    }
    file.close();


    return 0;
}

