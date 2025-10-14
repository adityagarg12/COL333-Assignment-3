// metro_decoder.cpp
// Companion decoder for metro_encoder.cpp
// Compile: g++ -std=c++17 -O2 metro_decoder.cpp -o run2
// Usage: ./run2 <basename>

#include <bits/stdc++.h>
using namespace std;
struct Point { int x, y; };

// Helper for sgn function (sign of a number)
int sgn(int val) {
    return (0 < val) - (val < 0);
}

// Calculate minimum turns required to reach end from current position
// considering the incoming direction
int min_turns_required_with_direction(int curr_x, int curr_y, int end_x, int end_y, int incoming_dir) {
    int dx = end_x - curr_x;
    int dy = end_y - curr_y;

    if (dx == 0 && dy == 0) return 0; // Already at destination

    // Direction mapping: 0=R, 1=L, 2=D, 3=U
    // incoming_dir is the direction FROM which we came TO current cell

    int min_turns = 0;

    if (incoming_dir == 0) { // Coming from Right (going Left)
        if (dy == 0) {
            // Only horizontal movement needed
            min_turns = (dx < 0) ? 3 : 0; // Going back right needs 3 turns, continuing left needs 0
        } else {
            // Both horizontal and vertical movement needed
            min_turns = (dx < 0) ? 2 : 1; // Going back right needs 2 turns, continuing needs 1
        }
    } else if (incoming_dir == 1) { // Coming from Left (going Right) 
        if (dy == 0) {
            // Only horizontal movement needed
            min_turns = (dx > 0) ? 3 : 0; // Going back left needs 3 turns, continuing right needs 0
        } else {
            // Both horizontal and vertical movement needed  
            min_turns = (dx > 0) ? 2 : 1; // Going back left needs 2 turns, continuing needs 1
        }
    } else if (incoming_dir == 2) { // Coming from Down (going Up)
        if (dx == 0) {
            // Only vertical movement needed
            min_turns = (dy < 0) ? 3 : 0; // Going back down needs 3 turns, continuing up needs 0
        } else {
            // Both horizontal and vertical movement needed
            min_turns = (dy < 0) ? 2 : 1; // Going back down needs 2 turns, continuing needs 1
        }
    } else if (incoming_dir == 3) { // Coming from Up (going Down)
        if (dx == 0) {
            // Only vertical movement needed
            min_turns = (dy > 0) ? 3 : 0; // Going back up needs 3 turns, continuing down needs 0
        } else {
            // Both horizontal and vertical movement needed
            min_turns = (dy > 0) ? 2 : 1; // Going back up needs 2 turns, continuing needs 1
        }
    }

    return min_turns;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: ./run2 <basename>\n";
        return 1;
    }

    string base = argv[1];
    string cityfile = base + ".city";
    string satout = base + ".satoutput";
    string mapout = base + ".metromap";

    // ---- Read city file ----
    ifstream fin(cityfile);
    if (!fin.is_open()) {
        cerr << "Cannot open " << cityfile << "\n";
        return 1;
    }

    int scenario;
    fin >> scenario;
    int N, M, K, J, P = 0;
    if (scenario == 1) fin >> N >> M >> K >> J;
    else fin >> N >> M >> K >> J >> P;

    vector<Point> starts(K), ends(K);
    for (int k = 0; k < K; k++) {
        fin >> starts[k].x >> starts[k].y >> ends[k].x >> ends[k].y;
    }

    vector<Point> popular;
    if (scenario == 2) {
        for (int i = 0; i < P; i++) {
            Point p; fin >> p.x >> p.y; popular.push_back(p);
        }
    }
    fin.close();

    // ---- Read SAT output ----
    ifstream fsat(satout);
    if (!fsat.is_open()) {
        cerr << "Cannot open " << satout << "\n";
        return 1;
    }

    string first;
    if (!(fsat >> first)) {
        cerr << "Empty " << satout << "\n";
        return 1;
    }

    if (first == "UNSAT" || first == "0" || first == "No" || first == "NO") {
        ofstream fout(mapout);
        fout << "0\n";
        fout.close();
        cerr << "UNSAT -> wrote 0\n";
        return 0;
    }

    vector<int> assignments;
    int val;
    while (fsat >> val) {
        if (val != 0) assignments.push_back(val);
    }
    fsat.close();

    unordered_set<int> trueVars;
    for (int v : assignments)
        if (v > 0) trueVars.insert(v);

    // ---- Rebuild mapping (exact same order as encoder) ----
    int var_counter = 1;
    unordered_map<string, int> idmap;

    auto getid = [&](const string &key) -> int {
        auto it = idmap.find(key);
        if (it != idmap.end()) return it->second;
        idmap[key] = var_counter++;
        return idmap[key];
    };

    auto occ_name = [&](int k, int x, int y) {
        return string("occ_") + to_string(k) + "_" + to_string(x) + "_" + to_string(y);
    };
    auto edge_name = [&](int k, int x1, int y1, int x2, int y2) {
        return string("e_") + to_string(k) + "_" + to_string(x1) + "_" + to_string(y1)
               + "_" + to_string(x2) + "_" + to_string(y2);
    };
    auto turn_name = [&](int k, int x, int y) {
        return string("t_") + to_string(k) + "_" + to_string(x) + "_" + to_string(y);
    };
    auto remain_name = [&](int k, int x, int y, int j) {
        return string("remain_") + to_string(k) + "_" + to_string(x) + "_" + to_string(y) + "_" + to_string(j);
    };

    vector<pair<int,int>> dirs = {{1,0},{-1,0},{0,1},{0,-1}};
    auto in_bounds = [&](int x,int y){return x>=0 && x<N && y>=0 && y<M;};

    // STEP 1: Replicate first loop from encoder (lines 168-182)
    // 1) Link edges -> occupancy variables  
    for(int k=0;k<K;k++){
        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                // For each neighbor
                for(int di=0;di<4;di++){
                    int nx=x+dirs[di].first;
                    int ny=y+dirs[di].second;
                    if(!in_bounds(nx,ny)) continue;
                    string evar = edge_name(k,x,y,nx,ny);
                    getid(evar);  // ev = cnf.getid(evar);
                    getid(occ_name(k,x,y));    // occ1 = cnf.getid(occ_name(k,x,y));
                    getid(occ_name(k,nx,ny));  // occ2 = cnf.getid(occ_name(k,nx,ny));

                    string rev_evar = edge_name(k,nx,ny,x,y);
                    getid(rev_evar);  // rev_ev = cnf.getid(edge_name(k,nx,ny,x,y));
                }
            }
        }
    }

    // STEP 2: Replicate degree constraint loop (lines 189-196)
    // 2) For non-start/non-end cells: degree constraints
    for(int k=0;k<K;k++){
        auto s = starts[k];
        auto e = ends[k];
        // Ensure start & end cells are occupied
        getid(occ_name(k,s.x,s.y));  // cnf.getid(occ_name(k,s.first,s.second))
        getid(occ_name(k,e.x,e.y));  // cnf.getid(occ_name(k,e.first,e.second))
        
        // OPTIMIZATION: Explicitly forbid other metros from occupying this metro's start/end cells
        if (K > 1) {
            for(int other_k = 0; other_k < K; ++other_k) {
                if (other_k != k) {
                    // Forbid other metros from start cell
                    getid(occ_name(other_k, s.x, s.y));  // cnf.getid(occ_name(other_k, s.first, s.second))
                    // Forbid other metros from end cell
                    getid(occ_name(other_k, e.x, e.y));  // cnf.getid(occ_name(other_k, e.first, e.second))
                }
            }
        }

        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                getid(occ_name(k,x,y));  // int occ = cnf.getid(occ_name(k,x,y));
                // collect outgoing edges from (x,y)
                for(int di=0;di<4;di++){
                    int nx=x+dirs[di].first;
                    int ny=y+dirs[di].second;
                    if(!in_bounds(nx,ny)) continue;
                    getid(edge_name(k,x,y,nx,ny));  // int outv = cnf.getid(edge_name(k,x,y,nx,ny));
                    getid(edge_name(k,nx,ny,x,y));  // int inv = cnf.getid(edge_name(k,nx,ny,x,y));
                }
            }
        }
    }

    // STEP 3: Replicate uniqueness constraint loop (matches SAT solver's pairwise encoding)
    // 3) If a cell is occupied, it must be occupied by at most one metro using pairwise constraints
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            // If there's more than one metro, we need to add a constraint.
            if (K > 1) {
                // Collect all occupancy variables for this specific cell (x,y).
                for (int k = 0; k < K; ++k) {
                    getid(occ_name(k, x, y));  // cnf.getid(occ_name(k, x, y))
                }
                
                // NOTE: Pairwise constraints don't create auxiliary variables,
                // so no additional variable generation needed here
            }
        }
    }

    // STEP 4: Replicate turn-remaining variable generation (matches SAT solver lines 530-580)
    // 5) Turn-remaining approach: Replace Sinz encoding with turn budget tracking
    for(int k=0;k<K;k++){
        int end_x = ends[k].x;
        int end_y = ends[k].y;
        int start_x = starts[k].x;  
        int start_y = starts[k].y;

        // 5.1) Start cell initialization: remain[k][start_x][start_y][J] = true
        getid(remain_name(k, start_x, start_y, J));

        // 5.2) For each cell, implement turn-remaining constraints
        for(int x = 0; x < N; x++) {
            for(int y = 0; y < M; y++) {
                // Skip cells that are definitely not on any valid path
                if(x == start_x && y == start_y) continue; // Start cell already handled

                // Get all possible incoming edges for this cell
                vector<pair<int,int>> incoming_edges; // pair<edge_var, direction>
                for(int di = 0; di < 4; di++) {
                    int prev_x = x + dirs[di].first;
                    int prev_y = y + dirs[di].second; 
                    if(!in_bounds(prev_x, prev_y)) continue;

                    int edge_var = getid(edge_name(k, prev_x, prev_y, x, y));
                    incoming_edges.push_back({edge_var, di});
                }

                // Get all possible outgoing edges from this cell
                vector<pair<int,int>> outgoing_edges; // pair<edge_var, direction>
                for(int di = 0; di < 4; di++) {
                    int next_x = x + dirs[di].first;
                    int next_y = y + dirs[di].second;
                    if(!in_bounds(next_x, next_y)) continue;

                    int edge_var = getid(edge_name(k, x, y, next_x, next_y));
                    outgoing_edges.push_back({edge_var, di});
                }

                if(incoming_edges.empty()) continue;

                // For each number of turns remaining j and each incoming edge
                for(int j = 0; j <= J; j++) {
                    getid(remain_name(k, x, y, j));

                    for(auto &inc_edge : incoming_edges) {
                        int inc_var = inc_edge.first;
                        int inc_dir = inc_edge.second;

                        // Calculate minimum turns required from this cell to end
                        int min_turns_req = min_turns_required_with_direction(x, y, end_x, end_y, inc_dir);

                        // Check if it's feasible to reach the end with j turns remaining
                        if(j < min_turns_req) {
                            // Not enough turns to reach end from here, so remain[k][x][y][j] must be false
                            // when inc_var is true (this clause is: ¬inc_var ∨ ¬remain_var)
                            // This is handled by SAT solver constraints, decoder just generates variables
                            continue;
                        }

                        // Generate variables for outgoing edges and turn consumption
                        for(auto &out_edge : outgoing_edges) {
                            int out_var = out_edge.first;
                            int out_dir = out_edge.second;

                            // Check if this is a turn or straight move
                            // Direction mapping: 0=R, 1=L, 2=D, 3=U
                            // Opposite directions: 0<->1, 2<->3
                            int opposite_of_inc;
                            if(inc_dir == 0) opposite_of_inc = 1;
                            else if(inc_dir == 1) opposite_of_inc = 0;
                            else if(inc_dir == 2) opposite_of_inc = 3;
                            else opposite_of_inc = 2;

                            bool is_straight = (out_dir == opposite_of_inc);

                            if(is_straight) {
                                // Straight move: turns remaining stays same
                                int next_x = x + dirs[out_dir].first;
                                int next_y = y + dirs[out_dir].second;
                                if(in_bounds(next_x, next_y)) {
                                    getid(remain_name(k, next_x, next_y, j));
                                }
                            } else {
                                // Turn: consume one turn (j -> j-1)
                                if(j > 0) {
                                    int next_x = x + dirs[out_dir].first;
                                    int next_y = y + dirs[out_dir].second;
                                    if(in_bounds(next_x, next_y)) {
                                        getid(remain_name(k, next_x, next_y, j-1));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // STEP 6: Replicate popular cells loop (lines 369-377)
    // 7) Popular cells (scenario 2)
    if(scenario==2){
        for(auto &p : popular){
            int x=p.x,y=p.y;
            for(int k=0;k<K;k++){
                getid(occ_name(k,x,y));  // clause.push_back(cnf.getid(occ_name(k,x,y)));
            }
        }
    }

    // ---- Build graph of true edges ----
    vector<map<pair<int,int>, vector<pair<pair<int,int>,char>>>> adj(K);

    for (int k = 0; k < K; k++) {
        for (int x1 = 0; x1 < N; x1++) {
            for (int y1 = 0; y1 < M; y1++) {
                for (auto &d : dirs) {
                    int x2 = x1 + d.first;
                    int y2 = y1 + d.second;
                    if (!in_bounds(x2, y2)) continue;
                    string key = edge_name(k, x1, y1, x2, y2);
                    int id = idmap[key];
                    if (trueVars.count(id)) {
                        char dir;
                        if (d == dirs[0]) dir = 'R';
                        else if (d == dirs[1]) dir = 'L';
                        else if (d == dirs[2]) dir = 'D';
                        else dir = 'U';
                        adj[k][{x1, y1}].push_back({{x2, y2}, dir});
                    }
                }
            }
        }
    }

    // ---- Trace paths ----
    ofstream fout(mapout);
    if (!fout.is_open()) {
        cerr << "Cannot open " << mapout << "\n";
        return 1;
    }

    for (int k = 0; k < K; k++) {
        pair<int,int> cur = {starts[k].x, starts[k].y};
        pair<int,int> end = {ends[k].x, ends[k].y};
        set<pair<int,int>> visited;
        vector<char> moves;
        bool ok = true;

        visited.insert(cur);
        int steps = 0;
        while (cur != end && steps < N*M + 10) {
            auto it = adj[k].find(cur);
            if (it == adj[k].end() || it->second.empty()) {
                ok = false;
                break;
            }

            bool moved = false;
            for (auto &nxt : it->second) {
                if (!visited.count(nxt.first)) {
                    moves.push_back(nxt.second);
                    cur = nxt.first;
                    visited.insert(cur);
                    moved = true;
                    break;
                }
            }
            if (!moved) { ok = false; break; }
            steps++;
        }

        if (!ok || cur != end) {
            fout << "0\n";
            cerr << "Failed to reconstruct path for metro " << k << "\n";
            fout.close();
            return 0;
        }

        for (char c : moves) fout << c << " ";
        fout << "0\n";
    }

    fout.close();
    cerr << "Decoded successfully → " << mapout << "\n";
    return 0;
}


