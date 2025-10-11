// metro_decoder.cpp
// Companion decoder for metro_encoder.cpp
// Compile: g++ -std=c++17 -O2 metro_decoder.cpp -o run2
// Usage: ./run2 <basename>

#include <bits/stdc++.h>
using namespace std;
struct Point { int x, y; };

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

    // STEP 3: Replicate uniqueness constraint loop (lines 284-292)
    // 3) If a cell is occupied, it must be occupied by at most one metro
    for(int x=0;x<N;x++){
        for(int y=0;y<M;y++){
            for(int k1=0;k1<K;k1++){
                getid(occ_name(k1,x,y));  // int v1 = cnf.getid(occ_name(k1,x,y));
                for(int k2=k1+1;k2<K;k2++){
                    getid(occ_name(k2,x,y));  // int v2 = cnf.getid(occ_name(k2,x,y));
                }
            }
        }
    }

    // STEP 4: Replicate turn detection loop (lines 298-356) 
    // 5) Turn detection variables and Sinz auxiliary variables
    for(int k=0;k<K;k++){
        vector<string> t_vars_for_metro;  // collect turn var names for Sinz
        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                // skip start and end - turns at start are not counted
                if(x==starts[k].x && y==starts[k].y) continue;
                if(x==ends[k].x && y==ends[k].y) continue;

                vector<pair<string,int>> in_edges; // pair<var_name,dir>
                vector<pair<string,int>> out_edges;
                for(int di=0;di<4;di++){
                    int nx = x + dirs[di].first;
                    int ny = y + dirs[di].second;
                    if(!in_bounds(nx,ny)) continue;
                    string invar = edge_name(k,nx,ny,x,y);
                    string outvar = edge_name(k,x,y,nx,ny);
                    getid(invar);   // int inlit = cnf.getid(edge_name(k,nx,ny,x,y));
                    getid(outvar);  // int outlit = cnf.getid(edge_name(k,x,y,nx,ny));
                    in_edges.push_back({invar, di});
                    out_edges.push_back({outvar, di});
                }
                if(in_edges.empty() || out_edges.empty()) continue;
                string tvar = turn_name(k,x,y);
                getid(tvar);  // int tvar = cnf.getid(turn_name(k,x,y));
                t_vars_for_metro.push_back(tvar);
            }
        }

        // STEP 5: Replicate Sinz auxiliary variables for this metro
        // 6) Enforce at-most-J turns using Sinz encoding
        if(!t_vars_for_metro.empty()){
            int n = (int)t_vars_for_metro.size();
            if(J < n && J >= 0) {  // Only create Sinz vars if needed
                // Create Sinz auxiliary variables: s[i][j] with i in [1..n-1], j in [1..J]
                string aux_prefix = string("sinz_k_") + to_string(k);
                for (int i = 1; i <= n-1; ++i) {
                    for (int j = 1; j <= J; ++j) {
                        string key = aux_prefix + "_s_" + to_string(i) + "_" + to_string(j);
                        getid(key);  // s[i][j] = cnf.getid(key);
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


