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
map<int, map<int, map<int, int>>> forward_table;

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

void dijkstra(int source) {
	unordered_map<int, int> D;
	unordered_set<int> N;
	unordered_map<int, int> neighbors = edges[source];
	unordered_map<int, int> P;
	map<int, int> spanning_tree;

	N.insert(source);
	for(int target : nodes) {
		P[target] = source;
		if(source == target) {
			D[target] = 0;
		}
		else if(neighbors.count(target) > 0) {
			D[target] = edges[source][target];
		}
		else {
			D[target] = INT_MAX;
		}
	}

	int start = 0;
	int count = 0;
	while(N.size() < nodes.size() && count <= nodes.size()) {
		int next_node = INT_MAX;
		int cost_to_next_node = INT_MAX;

		for(int neighbor : nodes) {
			if(N.count(neighbor) <= 0) {
				int cost_to_this_node = D[neighbor];
				if(cost_to_this_node < cost_to_next_node) {
					next_node = neighbor;
					cost_to_next_node = cost_to_this_node;
				}
			}
		}
		if(next_node < INT_MAX) {
			N.insert(next_node);
			spanning_tree[start] = next_node;
			start++;
			//Update cost for all adjacent nodes of next_node that not in N
			unordered_map<int, int> neighbors_of_next_node = edges[next_node];	
			for(auto i = neighbors_of_next_node.begin(); i != neighbors_of_next_node.end(); i++) {
				int neighbor_of_next_node = i -> first;
				int cost_to_neighbor = i -> second;
				if(D[next_node] + cost_to_neighbor < D[neighbor_of_next_node]) {
					D[neighbor_of_next_node] = D[next_node] + cost_to_neighbor;
					P[neighbor_of_next_node] = next_node;
				}
			}
		}
		count++;
	}
	for(int i = 0; i < spanning_tree.size(); i++) {
		int temp = spanning_tree[i];
		if(P[temp] == source) {
			forward_table[source][temp][0] = temp;
		}
		else {
			forward_table[source][temp][0] = forward_table[source][P[temp]][0];
		}
		forward_table[source][temp][1] = D[temp];
	}
	forward_table[source][source][0] = source;
	forward_table[source][source][1] = 0;



}

void dijkstra() {
	forward_table.clear();
	for(int source : nodes) {
		dijkstra(source);
	}
}

void printForwardTable() {
	ofstream outfile;
	outfile.open("output.txt", ios_base::app);
	for(auto i = forward_table.begin(); i != forward_table.end(); i++) {
		map<int, map<int, int>> temp = i -> second;
		//cout << i -> first;
		for(auto j = temp.begin(); j != temp.end(); j++) {
			map<int, int> temp2 = j -> second;
			//cout<< "Destination: ";
			//cout << j -> first;
			outfile << j -> first;
			outfile << " "; 
			//cout << temp2[0];
			outfile << temp2[0];
			outfile << " ";
			outfile << temp2[1];
			outfile << "\n";
		}
	}
	outfile.close();
}

void printMessage(int source, int target) {
	ofstream outfile;
	outfile.open("output.txt", ios_base::app);
	if(forward_table.count(source) <= 0 || forward_table[source].count(target) <= 0) {
		outfile << "from " << source << " to " << target << " cost infinite hops unreachable message ";
	}
	else if(source == target) {
		int cost = 0;
		outfile << "from " << source << " to " << target << " cost " << cost << " hops " << source << " message "; 
	}
	else {
		map<int, int> next_op_and_cost = forward_table[source][target];
		int cost = next_op_and_cost[1];
		queue<int> path;
		int curr = source;
		while(curr != target) {
			path.push(curr);
			curr = next_op_and_cost[0];
			next_op_and_cost = forward_table[curr][target];

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
    dijkstra();
    printForwardTable();
    ifstream file;
    file.open(argv[3]);
    int node1;
    int node2;
    int cost;
    //Add missing steps
    ifstream message_before_change(argv[2]);
    string line_curr;
    if(message_before_change.is_open()) {
	    int s;
	    int t;
	    while(getline(message_before_change, line_curr)) {
		    stringstream kk;
		    kk.str(line_curr);
		    kk >> s >> t;
		    int split1 = line_curr.find(" ");
		    string post_split1 = line_curr.substr(split1 + 1);
		    int split2 = post_split1.find(" ");
		    string post_split2 = post_split1.substr(split2 + 1);
		    printMessage(s, t);
		    ofstream out;
		    out.open("output.txt", ios_base::app);
		    out << post_split2 << "\n";
                    out.close();
	    }
	    message_before_change.close();
    }



    int change_num = 1;
    while(file >> node1 >> node2 >> cost) {
	    ofstream outfile_change_num;
	    outfile_change_num.open("output.txt", ios_base::app);
	    outfile_change_num << "---At this point, " << change_num << " change is applied" << "\n";
	    outfile_change_num.close();
	    if(cost == -999) {
		    edges[node1].erase(node2);
		    edges[node2].erase(node1);
	    }
	    else {
		    edges[node1][node2] = cost;
		    edges[node2][node1] = cost;
	    }
	    dijkstra();
	    printForwardTable();
	    ifstream message(argv[2]);
	    string line;
	    if(message.is_open()) {
		    int source;
		    int target;
		    while(getline(message, line)) {
			    //cout << line << "\n";
			    stringstream ss;
			    ss.str(line);
			    ss >> source >> target;
			    int split_one = line.find(" ");
			    string after_first_split = line.substr(split_one + 1);
			    int split_two = after_first_split.find(" ");
			    string after_second_split = after_first_split.substr(split_two + 1);
			    //cout << after_second_split;
			    //cout << source;
			    //cout << "\n";
			    //cout << target; 
			    printMessage(source, target);
			    ofstream outfile;
			    outfile.open("output.txt", ios_base::app);
			    outfile << after_second_split << "\n";
			    outfile.close();
		    }
		    message.close();
	    }
	    change_num++;
    }
    file.close();


    return 0;
}

