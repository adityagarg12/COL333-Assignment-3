// metro_encoder.cpp
// Produces CNF for the Metro SAT assignment (Part I and II).
// Compile: g++ -std=c++11 -O2 metro_encoder.cpp -o run1

#include <bits/stdc++.h>
using namespace std;

// ---------- Utility for variable mapping and CNF collection ----------
struct CNF {
    vector<vector<int>> clauses;
    int var_counter = 1;
    unordered_map<string,int> idmap;

    int newvar() { return var_counter++; }
    int getid(const string &key) {
        auto it = idmap.find(key);
        if (it != idmap.end()) return it->second;
        int v = newvar();
        idmap[key] = v;
        return v;
    }
    void add_clause(const vector<int>& c) {
        clauses.push_back(c); // Always add clause, even if empty (empty = UNSAT)
    }
    void add_clause(int a) { clauses.push_back(vector<int>{a}); }
    void add_clause(int a,int b) { clauses.push_back(vector<int>{a,b}); }
    void add_clause(int a,int b,int c) { clauses.push_back(vector<int>{a,b,c}); }
    void write_dimacs(const string &outfn) {
        ofstream f(outfn);
        if(!f.is_open()){ cerr<<"Cannot open "<<outfn<<"\n"; exit(1); }
        f << "p cnf " << (var_counter-1) << " " << clauses.size() << "\n";
        for (auto &cl : clauses) {
            for (int lit : cl) f << lit << " ";
            f << "0\n";
        }
        f.close();
        cerr << "Wrote " << clauses.size() << " clauses, " << (var_counter-1) << " vars -> " << outfn << "\n";
    }
};


// Helper function remains the same
int min_turns(pair<int, int> p1, pair<int, int> p2) {
    if (p1.first == p2.first || p1.second == p2.second) {
        return 0;
    }
    return 1;
}

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

// Helper function to create remain variable name
string remain_name(int k, int x, int y, int j) {
    return string("remain_") + to_string(k) + "_" + to_string(x) + "_" + to_string(y) + "_" + to_string(j);
}

// ---------- Sinz sequential counter (at-most-k) ----------
// Enforces: sum(a_1..a_n) <= K.
// Returns vector of created aux var names (for debugging if needed).
// We follow Sinz (2005) compact encoding. Produces O(n*K) vars and clauses.
// void sinz_atmost_k(CNF &cnf, const vector<int> &a, int K, const string &aux_prefix) {
//     int n = (int)a.size();
//     if (K >= n) return; // trivially satisfied
//     if (K < 0) { // unsatisfiable
//         // add clause (false)
//         cnf.add_clause(vector<int>{});
//         return;
//     }
//     // s[i][j] with i in [1..n-1], j in [1..K]
//     vector<vector<int>> s(n, vector<int>(K+1, 0)); // we use indices [1..n-1]/[1..K]
//     for (int i = 1; i <= n-1; ++i) {
//         for (int j = 1; j <= K; ++j) {
//             string key = aux_prefix + "_s_" + to_string(i) + "_" + to_string(j);
//             s[i][j] = cnf.getid(key);
//         }
//     }

//     // Clauses:
//     // 1) (¬a1 ∨ s1,1)
//     cnf.add_clause(vector<int>{ -a[0], s[1][1] });
//     // 2) for j=2..K: (¬s1,j)  (first row)
//     for (int j = 2; j <= K; ++j) cnf.add_clause(vector<int>{ -s[1][j] });

//     // For i = 2..n-1:
//     // (¬ai ∨ s_i,1)
//     // (¬s_{i-1,1} ∨ s_{i,1})
//     // for j=2..K: (¬s_{i-1,j} ∨ s_{i,j})
//     // for j=1..K: (¬ai ∨ ¬s_{i-1,j})  -- this is the "limit" clause
//     for (int i = 2; i <= n-1; ++i) {
//         cnf.add_clause(vector<int>{ -a[i-1], s[i][1] });
//         cnf.add_clause(vector<int>{ -s[i-1][1], s[i][1] });
//         for (int j = 2; j <= K; ++j) {
//             cnf.add_clause(vector<int>{ -s[i-1][j], s[i][j] });
//         }
//         for (int j = 1; j <= K; ++j) {
//             cnf.add_clause(vector<int>{ -a[i-1], -s[i-1][j] });
//         }
//     }

//     // For the last variable a_n:
//     // (¬a_n ∨ ¬s_{n-1,K})
//     cnf.add_clause(vector<int>{ -a[n-1], -s[n-1][K] });
// }

// ---------- Sinz sequential counter (at-most-k) ----------
// Enforces: sum(a_1..a_n) <= K.
// We follow Sinz (2005) compact encoding. Produces O(n*K) vars and clauses.
void sinz_atmost_k(CNF &cnf, const vector<int> &a, int K, const string &aux_prefix) {
    int n = (int)a.size();
    if (K >= n) return; // trivially satisfied
    if (K < 0) { // unsatisfiable
        cnf.add_clause(vector<int>{});
        return;
    }
    if (n == 0) return;

    // s[i][j] is true iff sum of first i variables (a_0..a_{i-1}) is at least j.
    // Matrix s is n x (K+1). We use indices s[i][j] for i=0..n-1, j=1..K
    vector<vector<int>> s(n, vector<int>(K+1, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j <= K; ++j) {
            string key = aux_prefix + "_s_" + to_string(i) + "_" + to_string(j);
            s[i][j] = cnf.getid(key);
        }
    }

    // --- FIX: The entire logic below is new and correct. ---

    // Base case: i = 0 (for the first variable a[0])
    // if a[0] is true, then sum is at least 1.  (¬a_0 ∨ s_{0,1})
    cnf.add_clause({ -a[0], s[0][1] });
    // sum of first variable can't be >= 2. (¬s_{0,j} for j>1)
    for (int j = 2; j <= K; ++j) {
        cnf.add_clause({ -s[0][j] });
    }

    // Recursive step: i = 1 to n-1
    for (int i = 1; i < n; ++i) {
        // If a[i] is true, sum is at least 1. (¬a_i ∨ s_{i,1})
        cnf.add_clause({-a[i], s[i][1]});
        // If sum up to i-1 was >= 1, sum up to i is also >= 1. (¬s_{i-1,1} ∨ s_{i,1})
        cnf.add_clause({-s[i-1][1], s[i][1]});

        for (int j = 2; j <= K; ++j) {
            // Propagate existing sums: if sum was >=j before, it still is. (¬s_{i-1,j} ∨ s_{i,j})
            cnf.add_clause({-s[i-1][j], s[i][j]});
            // CRITICAL INCREMENT STEP: if a[i] is true and sum was >= j-1, it is now >= j.
            // (¬a_i ∨ ¬s_{i-1,j-1} ∨ s_{i,j})
            cnf.add_clause({-a[i], -s[i-1][j-1], s[i][j]});
        }
    }

    // Cardinality constraint: For every variable a[i], if the sum of the
    // previous i-1 variables was already K, then a[i] must be false.
    // (¬a_i ∨ ¬s_{i-1,K})
    for (int i = 1; i < n; ++i) {
        cnf.add_clause({ -a[i], -s[i-1][K] });
    }
}


// ---------- Main encoding functions ----------
int main(int argc,char**argv){
    if(argc!=2){
        cerr<<"Usage: "<<argv[0]<<" <input.city>\n";
        return 1;
    }

    // Read city file
    string infile = argv[1];
    ifstream fin(infile);
    if(!fin.is_open()){ cerr<<"Cannot open "<<infile<<"\n"; return 1;}
    string line;
    vector<string> lines;
    while (getline(fin,line)) {
        // trim trailing whitespace
        while(!line.empty() && isspace((unsigned char)line.back())) line.pop_back();
        if(!line.empty()) lines.push_back(line);
    }
    fin.close();
    if(lines.empty()){ cerr<<"Empty city file\n"; return 1; }

    int idx=0;
    int scenario = stoi(lines[idx++]);
    istringstream iss(lines[idx++]);
    int N,M,K,J,P=0;
    if(scenario==1){
        iss>>N>>M>>K>>J;
    } else {
        iss>>N>>M>>K>>J>>P;
    }
    vector<pair<int,int>> starts(K), ends(K);
    for(int k=0;k<K;k++){
        istringstream is(lines[idx++]);
        int sx,sy,ex,ey; is>>sx>>sy>>ex>>ey;
        starts[k]={sx,sy}; ends[k]={ex,ey};
    }
    vector<pair<int,int>> popular;
    if(scenario==2){
        istringstream is(lines[idx++]);
        vector<int> tmp;
        int v;
        while(is>>v) tmp.push_back(v);
        for(int i=0;i<(int)tmp.size(); i+=2) popular.emplace_back(tmp[i], tmp[i+1]);
    }

    // Basic checks (format checker expects starts/ends unique etc.)
    {
        set<pair<int,int>> sset, eset;
        for(auto &p:starts) sset.insert(p);
        for(auto &p:ends) eset.insert(p);
        if((int)sset.size()!=K || (int)eset.size()!=K){
            cerr<<"Duplicate starts/ends in input (invalid city file)\n";
            return 1;
        }
        for(auto &p:starts) if(eset.count(p)){ cerr<<"A start equals some end (invalid)\n"; return 1;}
    }

    CNF cnf;

    auto occ_name = [&](int k,int x,int y){
        return string("occ_")+to_string(k)+"_"+to_string(x)+"_"+to_string(y);
    };
    auto edge_name = [&](int k,int x1,int y1,int x2,int y2){
        return string("e_")+to_string(k)+"_"+to_string(x1)+"_"+to_string(y1)+"_"+to_string(x2)+"_"+to_string(y2);
    };
    auto turn_name = [&](int k,int x,int y){
        return string("t_")+to_string(k)+"_"+to_string(x)+"_"+to_string(y);
    };

    // Direction helpers
    vector<pair<int,int>> dirs = {{1,0},{-1,0},{0,1},{0,-1}}; // R,L,D,U
    
    // dir index mapping: 0:R,1:L,2:D,3:U
    auto in_bounds = [&](int x,int y){ return x>=0 && x<N && y>=0 && y<M; };

    // Create variables for occ and edges
    // We'll lazily create variables with cnf.getid when used.

    // 1) Link edges -> occupancy variables (if an edge is used, both endpoints occupied)
    for(int k=0;k<K;k++){
        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                // For each neighbor
                for(int di=0;di<4;di++){
                    int nx=x+dirs[di].first;
                    int ny=y+dirs[di].second;
                    if(!in_bounds(nx,ny)) continue;
                    string evar = edge_name(k,x,y,nx,ny);
                    int ev = cnf.getid(evar);
                    int occ1 = cnf.getid(occ_name(k,x,y));
                    int occ2 = cnf.getid(occ_name(k,nx,ny));
                    // e -> occ(u) and occ(v): (¬e ∨ occ_u), (¬e ∨ occ_v)
                    cnf.add_clause(vector<int>{ -ev, occ1 });
                    cnf.add_clause(vector<int>{ -ev, occ2 });



                    int rev_ev = cnf.getid(edge_name(k,nx,ny,x,y));
                    cnf.add_clause(vector<int>{ -ev, -rev_ev });


                }
            }
        }
    }

    // 2) For non-start/non-end cells: if occ then exactly one incoming edge and one outgoing edge.
    // For start: occ(start) must be true, indegree 0, outdegree 1.
    // For end: occ(end) must be true, indegree 1, outdegree 0.
    for(int k=0;k<K;k++){
        auto s = starts[k];
        auto e = ends[k];
        // Ensure start & end cells are occupied by this metro
        cnf.add_clause(vector<int>{ cnf.getid(occ_name(k,s.first,s.second))});
        cnf.add_clause(vector<int>{ cnf.getid(occ_name(k,e.first,e.second))});
        
        // OPTIMIZATION: Explicitly forbid other metros from occupying this metro's start/end cells
        if (K > 1) {
            for(int other_k = 0; other_k < K; ++other_k) {
                if (other_k != k) {
                    // Forbid other metros from start cell
                    cnf.add_clause(vector<int>{ -cnf.getid(occ_name(other_k, s.first, s.second)) });
                    // Forbid other metros from end cell
                    cnf.add_clause(vector<int>{ -cnf.getid(occ_name(other_k, e.first, e.second)) });
                }
            }
        }

        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                int occ = cnf.getid(occ_name(k,x,y));
                pair<int, int> c = {x, y};

                // --- NEW, STRONGER REACHABILITY CHECK ---
                int h_segments = 0;
                if (s.first != c.first || e.first != c.first) { // Needs horizontal movement
                    h_segments = 1;
                    // Check for horizontal double-back
                    if (sgn(c.first - s.first) * sgn(e.first - c.first) == -1) {
                        h_segments = 2;
                    }
                }

                int v_segments = 0;
                if (s.second != c.second || e.second != c.second) { // Needs vertical movement
                    v_segments = 1;
                    // Check for vertical double-back
                    if (sgn(c.second - s.second) * sgn(e.second - c.second) == -1) {
                        v_segments = 2;
                    }
                }

                // A path is an alternating sequence of H and V segments.
                // Turns happen at the transition between segments.
                // E.g., H-V-H-V requires 3 turns.
                int lower_bound_turns = 0;
                if (h_segments == 0 && v_segments == 0) {
                    // No movement needed (start == current == end) - should not happen in practice
                    lower_bound_turns = 0;
                } else if (h_segments == 0 || v_segments == 0) {
                    // Pure horizontal or pure vertical movement - no turns needed
                    lower_bound_turns = 0;
                } else {
                    // Both horizontal and vertical movement required
                    // Formula: h_segments + v_segments - 1 gives the minimum turns
                    // for alternating H-V-H-V... pattern
                    lower_bound_turns = h_segments + v_segments - 1;
                    
                    // Special case: when both h_segments=2 and v_segments=2,
                    // the minimum is actually 3 turns (e.g., H-V-H-V pattern)
                    // But our formula gives 2+2-1=3, which is already correct!
                    // So no special case needed.
                }


                if (lower_bound_turns > J) {
                    int occ_var = cnf.getid(occ_name(k, x, y));
                    cnf.add_clause(-occ_var);
                    continue; 
                }

                // collect outgoing edges from (x,y)
                vector<int> out_edges;
                vector<int> in_edges;
                for(int di=0;di<4;di++){
                    int nx=x+dirs[di].first;
                    int ny=y+dirs[di].second;
                    if(!in_bounds(nx,ny)) continue;
                    int outv = cnf.getid(edge_name(k,x,y,nx,ny));
                    out_edges.push_back(outv);
                    int inv = cnf.getid(edge_name(k,nx,ny,x,y));
                    in_edges.push_back(inv);
                }

                bool is_start = (x==s.first && y==s.second);
                bool is_end = (x==e.first && y==e.second);

                // Early check: if this is neither start nor end, and has insufficient neighbors for a path
                if (!is_start && !is_end && (in_edges.size() < 2 && out_edges.size() < 2)) {
                    // Interior cells need at least 2 connections to form a path through them
                    // If this cell has only 1 neighbor total, it can't be part of an interior path
                    if (in_edges.size() + out_edges.size() < 2) {
                        cnf.add_clause(vector<int>{ -occ });
                        continue;
                    }
                }

                if(is_start && is_end){
                    // degenerate case start==end (but spec forbids). We'll still be safe:
                    // require occ true, no incoming, no outgoing.
                    for(int v: in_edges) cnf.add_clause(vector<int>{ -v }); // no incoming
                    for(int v: out_edges) cnf.add_clause(vector<int>{ -v }); // no outgoing
                    continue;
                }

                if(is_start){
                    // start: indegree 0, exactly one outgoing
                    for(int v: in_edges) cnf.add_clause(vector<int>{ -v });
                    if(!out_edges.empty()){
                        // occ -> exactly one outgoing (combined constraint)
                        vector<int> clause; clause.push_back(-occ);
                        for(int v: out_edges) clause.push_back(v);
                        cnf.add_clause(clause);
                        // at-most-one outgoing: pairwise
                        for(size_t i=0;i<out_edges.size();++i)
                            for(size_t j=i+1;j<out_edges.size();++j)
                                cnf.add_clause(vector<int>{ -out_edges[i], -out_edges[j] });
                    } else {
                        cnf.add_clause(vector<int>{});
                    }
                } else if(is_end){
                    // end: outdegree 0, exactly one incoming
                    for(int v: out_edges) cnf.add_clause(vector<int>{ -v });
                    if(!in_edges.empty()){
                        // occ -> exactly one incoming (combined constraint)
                        vector<int> clause; clause.push_back(-occ);
                        for(int v: in_edges) clause.push_back(v);
                        cnf.add_clause(clause);
                        // at-most-one incoming
                        for(size_t i=0;i<in_edges.size();++i)
                            for(size_t j=i+1;j<in_edges.size();++j)
                                cnf.add_clause(vector<int>{ -in_edges[i], -in_edges[j] });
                    } else {
                        cnf.add_clause(vector<int>{});
                    }
                } else {
                    // interior: if occ then exactly one incoming and one outgoing
                    // occ -> (OR in_edges)  and occ -> (OR out_edges)
                    if(in_edges.empty() || out_edges.empty()){
                        // can't have a path through a cell with no neighbors => occ must be false
                        cnf.add_clause(vector<int>{ -occ });
                    } else {
                        vector<int> cin; cin.push_back(-occ);
                        for(int v: in_edges) cin.push_back(v);
                        cnf.add_clause(cin);
                        vector<int> cout; cout.push_back(-occ);
                        for(int v: out_edges) cout.push_back(v);
                        cnf.add_clause(cout);

                        // at-most-one incoming, at-most-one outgoing
                        for(size_t i=0;i<in_edges.size();++i)
                            for(size_t j=i+1;j<in_edges.size();++j)
                                cnf.add_clause(vector<int>{ -in_edges[i], -in_edges[j] });
                        for(size_t i=0;i<out_edges.size();++i)
                            for(size_t j=i+1;j<out_edges.size();++j)
                                cnf.add_clause(vector<int>{ -out_edges[i], -out_edges[j] });
                    }
                }
            }
        }
    }

    // 3) If a cell is occupied, it must be occupied by at most one metro (global uniqueness)
    // for(int x=0;x<N;x++){
    //     for(int y=0;y<M;y++){
    //         for(int k1=0;k1<K;k1++){
    //             int v1 = cnf.getid(occ_name(k1,x,y));
    //             for(int k2=k1+1;k2<K;k2++){
    //                 int v2 = cnf.getid(occ_name(k2,x,y));
    //                 cnf.add_clause(vector<int>{ -v1, -v2 });
    //             }
    //         }
    //     }
    // }

    cout << "debug --- IGNORE ---\n";

    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < M; ++y) {
            // If there's more than one metro, we need to add a constraint.
            if (K > 1) {
                // Collect all occupancy variables for this specific cell (x,y).
                vector<int> occ_vars_for_cell;
                for (int k = 0; k < K; ++k) {
                    occ_vars_for_cell.push_back(cnf.getid(occ_name(k, x, y)));
                }

                // Use pairwise constraints instead of Sinz for better Lit/Cl ratio
                for(size_t i = 0; i < occ_vars_for_cell.size(); ++i) {
                    for(size_t j = i + 1; j < occ_vars_for_cell.size(); ++j) {
                        cnf.add_clause(vector<int>{ -occ_vars_for_cell[i], -occ_vars_for_cell[j] });
                    }
                }
            }
        }
    }


    cout << "debug2 --- IGNORE ---\n";

    // 4) Edges must form connected path: ensure no disjoint cycles -> the degree constraints above + unique start/end ensure simple path
    // The constraints so far imply each occ cell (except endpoints) has indeg=outdeg=1; start has outdeg=1 indeg=0; end has indeg=1 outdeg=0.
    // That prevents multiple disjoint cycles per metro because the starts/ends connect the path. (This is a standard encoding.)

// 5) Turn-remaining approach: Replace Sinz encoding with turn budget tracking
    for(int k = 0; k < K; k++) {
        int end_x = ends[k].first;
        int end_y = ends[k].second;
        int start_x = starts[k].first;  
        int start_y = starts[k].second;

        // 5.1) Start cell initialization: remain[k][start_x][start_y][J] = true
        int start_remain_var = cnf.getid(remain_name(k, start_x, start_y, J));
        cnf.add_clause(start_remain_var);

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

                    int edge_var = cnf.getid(edge_name(k, prev_x, prev_y, x, y));
                    incoming_edges.push_back({edge_var, di});
                }

                // Get all possible outgoing edges from this cell
                vector<pair<int,int>> outgoing_edges; // pair<edge_var, direction>
                for(int di = 0; di < 4; di++) {
                    int next_x = x + dirs[di].first;
                    int next_y = y + dirs[di].second;
                    if(!in_bounds(next_x, next_y)) continue;

                    int edge_var = cnf.getid(edge_name(k, x, y, next_x, next_y));
                    outgoing_edges.push_back({edge_var, di});
                }

                if(incoming_edges.empty()) continue;

                // For each number of turns remaining j and each incoming edge
                for(int j = 0; j <= J; j++) {
                    int remain_var = cnf.getid(remain_name(k, x, y, j));

                    for(auto &inc_edge : incoming_edges) {
                        int inc_var = inc_edge.first;
                        int inc_dir = inc_edge.second;

                        // Calculate minimum turns required from this cell to end
                        int min_turns_req = min_turns_required_with_direction(x, y, end_x, end_y, inc_dir);

                        // 5.3) Path feasibility check (your Point 1)
                        if(j < min_turns_req) {
                            // If turns remaining < min required, no outgoing edge can be true
                            for(auto &out_edge : outgoing_edges) {
                                int out_var = out_edge.first;
                                // inc_var ∧ remain_var → ¬out_var
                                // Equivalent to: ¬inc_var ∨ ¬remain_var ∨ ¬out_var
                                cnf.add_clause(vector<int>{-inc_var, -remain_var, -out_var});
                            }
                        } else {
                            // 5.4) Path continuation requirement  
                            // If turns remaining >= min required, at least one outgoing edge must be true
                            if(!outgoing_edges.empty()) {
                                vector<int> clause;
                                clause.push_back(-inc_var);
                                clause.push_back(-remain_var);
                                for(auto &out_edge : outgoing_edges) {
                                    clause.push_back(out_edge.first);
                                }
                                cnf.add_clause(clause);
                            }
                        }

                        // 5.5) Turn consumption constraints
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
                                // 5.6) Straight move: turns remaining stays same
                                int next_x = x + dirs[out_dir].first;
                                int next_y = y + dirs[out_dir].second;
                                if(in_bounds(next_x, next_y)) {
                                    int next_remain_var = cnf.getid(remain_name(k, next_x, next_y, j));
                                    // inc_var ∧ remain_var ∧ out_var → next_remain_var
                                    // Equivalent to: ¬inc_var ∨ ¬remain_var ∨ ¬out_var ∨ next_remain_var
                                    cnf.add_clause(vector<int>{-inc_var, -remain_var, -out_var, next_remain_var});
                                }
                            } else {
                                // 5.7) Turn: consume one turn (j -> j-1)
                                if(j > 0) {
                                    int next_x = x + dirs[out_dir].first;
                                    int next_y = y + dirs[out_dir].second;
                                    if(in_bounds(next_x, next_y)) {
                                        int next_remain_var = cnf.getid(remain_name(k, next_x, next_y, j-1));
                                        // inc_var ∧ remain_var ∧ out_var → next_remain_var
                                        // Equivalent to: ¬inc_var ∨ ¬remain_var ∨ ¬out_var ∨ next_remain_var
                                        cnf.add_clause(vector<int>{-inc_var, -remain_var, -out_var, next_remain_var});
                                    }
                                } else {
                                    // j = 0, cannot make a turn
                                    // inc_var ∧ remain_var → ¬out_var
                                    // Equivalent to: ¬inc_var ∨ ¬remain_var ∨ ¬out_var
                                    cnf.add_clause(vector<int>{-inc_var, -remain_var, -out_var});
                                }
                            }
                        }
                    }
                }
            }
        }

        // 5.8) End condition: any remain[k][end_x][end_y][j] can be true for j >= 0
        // No additional constraints needed - the path can end with any number of remaining turns
    }


     // 7) Popular cells (scenario 2)

    if(scenario==2){

        for(auto &p : popular){

            int x=p.first,y=p.second;

            vector<int> clause;

            for(int k=0;k<K;k++){

                clause.push_back(cnf.getid(occ_name(k,x,y)));

            }

            // at least one metro occupies popular cell

            cnf.add_clause(clause);

        }

    }

    // 8) Optional: ensure there is at least one path cell for each metro between start and end:
    // We already forced occ(start) and occ(end). It's possible a metro sets occ on start and end but no connecting edges if constraints allowed;
    // but our edge<->occ and degree constraints forbid that because start must have an outgoing edge and end must have an incoming edge.
    // So path connectivity is enforced.

    // Write CNF
    string outfn = infile;
    size_t pos = outfn.rfind(".city");
    if(pos!=string::npos) outfn.replace(pos,5,".satinput"); else outfn += ".satinput";
    cnf.write_dimacs(outfn);

    return 0;
}
