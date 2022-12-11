#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <sstream>
#include <cmath>
#include <iomanip>

using namespace std;


struct Node {
    int window;
    int backoff;
    int num_collision;
    int time_trans;
};

int N;
int L;
int M;
int T;
//vector<int> time_slot;
vector<int> range;
int channelOccupied = 0;


void readInput(char* filename) {
    ifstream file;
    file.open(filename);
    string line;
    string type;
    string data;
    regex pat_regex("[[:digit:]]+");
    while(getline(file, line)) {
        type = line.substr(0, 1);
        data = line.substr(2);
        if(type == "N") {
            N = stoi(data);
            //cout << "N: " << N << "\n";
        }
        else if(type == "L") {
            L = stoi(data);
            //cout << "L: " << L << "\n";
        }
        else if(type == "R") {
            for(sregex_iterator it(data.begin(), data.end(), pat_regex), end_it; it != end_it; ++it) {
                range.push_back(stoi(it -> str()));
            }
        }
        else if(type == "M") {
            M = stoi(data);
            //cout << "M: " << M << "\n";
        }
        else if(type == "T") {
            T = stoi(data);
            //cout << "T: " << T << "\n";
        }
    }
    /*for(int i = 0; i < time_slot.size(); i++) {
      //  cout << time_slot[i] << " ";
    }*/
}


map<int, Node> nodes;
int setBackoff(int id, int t, int r) {
    return (id + t) % r;
}

void initializeNodes() {
    for(int i = 0; i < N; i++) {
        Node n;
        n.window = 0;
        n.backoff = setBackoff(i, 0, range[0]);
        n.num_collision = 0;
        nodes[i] = n;
        nodes[i].time_trans = L;
    }
}



int simulate() {
    int used = 0;
    int curr_node = -1;
    for(int tick = 0; tick < T; tick ++) {
        if(channelOccupied == 0) {
            int num_ready = 0;
            vector<int> node_ready;
            for(int i = 0; i < nodes.size(); i++) {
                if(nodes[i].backoff == 0) {
                    node_ready.push_back(i);
                    num_ready ++;
                }
            }
            if(num_ready == 0) {
                for(int i = 0; i < nodes.size(); i++) {
                    nodes[i].backoff -= 1;
                }
            }
            else if(num_ready == 1) {
                int idx_ready = node_ready[0];
                nodes[idx_ready].window = range[0];
                nodes[idx_ready].num_collision = 0;
                curr_node = idx_ready;
                nodes[idx_ready].time_trans -= 1;
                used += 1;
                channelOccupied = 1;
                if(nodes[idx_ready].time_trans == 0) {
                    channelOccupied = 0;
                    nodes[idx_ready].window = range[0];
                    nodes[idx_ready].num_collision = 0;
                    nodes[idx_ready].backoff = setBackoff(idx_ready, tick + 1, range[0]);
                    nodes[idx_ready].time_trans = L;
                } 
            }
            else {
                for(int i = 0; i < node_ready.size(); i++) {
                    int idx_ready = node_ready[i];
                    nodes[idx_ready].num_collision += 1;
                    if(nodes[idx_ready].num_collision >= M) {
                        nodes[idx_ready].window = range[0];
                        nodes[idx_ready].num_collision = 0;
                        nodes[idx_ready].backoff = setBackoff(idx_ready, tick + 1, range[0]);
                    }
                    else {
                        nodes[idx_ready].window = range[nodes[idx_ready].num_collision];
                        nodes[idx_ready].backoff = setBackoff(idx_ready, tick + 1, nodes[idx_ready].window);
                    }
                }
            }
        }
        else {
            if(nodes[curr_node].time_trans > 0) {
                nodes[curr_node].time_trans -= 1;
                used += 1;
                if(nodes[curr_node].time_trans == 0) {
                    channelOccupied = 0;
                    nodes[curr_node].window = range[0];
                    nodes[curr_node].num_collision = 0;
                    nodes[curr_node].backoff = setBackoff(curr_node, tick + 1, range[0]);
                    nodes[curr_node].time_trans = L;
                }
            }
        }
    }
    /*while(tick < T) {
        int num_ready = 0;
        vector<int> node_ready;
        for(int i = 0; i < nodes.size(); i++) {
            if(nodes[i].backoff == 0) {
                node_ready.push_back(i);
                num_ready ++;
            }
        }
        if(num_ready == 0) {
            for(int i = 0; i < nodes.size(); i++) {
                nodes[i].backoff -= 1;
            }
            tick += 1;
            continue;
        }
        else if(num_ready == 1) {
            if(tick + L > T) {
                used = used + (T - tick);
                tick = tick + L;
            }
            else {
                used = used + L;
                tick = tick + L;
            }
            int idx_ready = node_ready[0];
            nodes[idx_ready].window = range[0];
            nodes[idx_ready].num_collision = 0;
            nodes[idx_ready].backoff = setBackoff(idx_ready, tick, range[0]);
            continue;
        }
        else {
            for(int i = 0; i < node_ready.size(); i++) {
                int idx_ready = node_ready[i];
                nodes[idx_ready].num_collision += 1;
                if(nodes[idx_ready].num_collision >= M) {
                    nodes[idx_ready].window = range[0];
                    nodes[idx_ready].num_collision = 0;
                    nodes[idx_ready].backoff = setBackoff(idx_ready, tick + 1, range[0]);
                }
                else {
                    nodes[idx_ready].window = range[nodes[idx_ready].num_collision];
                    nodes[idx_ready].backoff = setBackoff(idx_ready, tick + 1, nodes[idx_ready].window);
                }
            }
            tick ++;
            continue;
        }
    }*/
    return used;
}




int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fclose(fpOut);

    readInput(argv[1]);
    initializeNodes();
    int used = simulate();
    double result = (1.0 * (used)) / (T * 1.0);
    ofstream outfile;
    outfile.open("output.txt", ios_base::app);
    outfile << setprecision(2) << fixed;
    outfile << result;
    outfile.close();
}

