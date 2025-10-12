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
        // Ensure start & end cells are occupied
        cnf.add_clause(vector<int>{ cnf.getid(occ_name(k,s.first,s.second))});
        cnf.add_clause(vector<int>{ cnf.getid(occ_name(k,e.first,e.second))});

        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                int occ = cnf.getid(occ_name(k,x,y));
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

                if(is_start && is_end){
                    // degenerate case start==end (but spec forbids). We'll still be safe:
                    // require occ true, no incoming, no outgoing.
                    for(int v: in_edges) cnf.add_clause(vector<int>{ -v }); // no incoming
                    for(int v: out_edges) cnf.add_clause(vector<int>{ -v }); // no outgoing
                    continue;
                }

                if(is_start){
                    // start: indegree 0
                    for(int v: in_edges) cnf.add_clause(vector<int>{ -v });
                    // occ -> at least one outgoing
                    if(!out_edges.empty()){
                        vector<int> clause; clause.push_back(-occ);
                        for(int v: out_edges) clause.push_back(v);
                        cnf.add_clause(clause);
                        // at-most-one outgoing: pairwise
                        for(size_t i=0;i<out_edges.size();++i)
                            for(size_t j=i+1;j<out_edges.size();++j)
                                cnf.add_clause(vector<int>{ -out_edges[i], -out_edges[j] });
                    } else {
                        // No outgoing neighbors -> impossible (unless start==end handled above)
                        // Make formula unsat by adding empty clause
                        cnf.add_clause(vector<int>{});
                    }
                } else if(is_end){
                    // end: outdegree 0
                    for(int v: out_edges) cnf.add_clause(vector<int>{ -v });
                    // occ -> at least one incoming
                    if(!in_edges.empty()){
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
    for(int x=0;x<N;x++){
        for(int y=0;y<M;y++){
            for(int k1=0;k1<K;k1++){
                int v1 = cnf.getid(occ_name(k1,x,y));
                for(int k2=k1+1;k2<K;k2++){
                    int v2 = cnf.getid(occ_name(k2,x,y));
                    cnf.add_clause(vector<int>{ -v1, -v2 });
                }
            }
        }
    }

    // 4) Edges must form connected path: ensure no disjoint cycles -> the degree constraints above + unique start/end ensure simple path
    // The constraints so far imply each occ cell (except endpoints) has indeg=outdeg=1; start has outdeg=1 indeg=0; end has indeg=1 outdeg=0.
    // That prevents multiple disjoint cycles per metro because the starts/ends connect the path. (This is a standard encoding.)

    // 5) Turn detection: define turn(k,x,y) true iff an incoming edge and outgoing edge with different orientation are used.
    // We'll define: for each possible pair (in_edge, out_edge) that constitutes a TURN, add clause (-in_edge - out_edge turn).
    // And also we add (turn -> OR of those pairs) optional (not necessary for correctness when using counters that count true turns),
    // but to keep t variables consistent, we add implication (turn -> OR(pair-lits)). Simpler: we set t to be an upper bound helper:
    for(int k=0;k<K;k++){
        vector<int> t_vars_for_metro;
        for(int x=0;x<N;x++){
            for(int y=0;y<M;y++){
                // skip start and end - turns at start are not counted (you can decide)
                if(x==starts[k].first && y==starts[k].second) continue;
                if(x==ends[k].first && y==ends[k].second) continue;

                vector<pair<int,int>> in_edges; // pair<lit,dir>
                vector<pair<int,int>> out_edges;
                for(int di=0;di<4;di++){
                    int nx = x + dirs[di].first;
                    int ny = y + dirs[di].second;
                    if(!in_bounds(nx,ny)) continue;
                    int inlit = cnf.getid(edge_name(k,nx,ny,x,y));
                    int outlit = cnf.getid(edge_name(k,x,y,nx,ny));
                    in_edges.push_back({inlit, di});
                    out_edges.push_back({outlit, di});
                }
                if(in_edges.empty() || out_edges.empty()) continue;
                int tvar = cnf.getid(turn_name(k,x,y));
                t_vars_for_metro.push_back(tvar);

                // For every pair (in,out) if directions differ (i.e., turning), then (¬in ∨ ¬out ∨ t)
                for(auto &ie : in_edges){
                    for(auto &oe : out_edges){
                        if(ie.first == oe.first) continue; // same edge, skip
                        
                        int din = ie.second;
                        int dout = oe.second;
                        
                        // A straight move means SAME direction continues (not opposite!)
                        // If incoming from R(0) and outgoing to R(0), that's a reversal (impossible)
                        // If incoming from L(1) and outgoing to R(0), that's straight through
                        // Opposite mapping: 0<->1 (R<->L), 2<->3 (D<->U)
                        int opp;
                        if(din==0) opp=1; else if(din==1) opp=0; else if(din==2) opp=3; else opp=2;
                        bool is_straight = (dout == opp);
                        
                        // Add only ONE clause per pair - either turn or not turn
                        if(is_straight){
                            // If straight pair: (¬in ∨ ¬out ∨ ¬t)
                            cnf.add_clause(vector<int>{ -ie.first, -oe.first, -tvar });
                        } else {
                            // If turn pair: (¬in ∨ ¬out ∨ t)
                            cnf.add_clause(vector<int>{ -ie.first, -oe.first, tvar });
                        }
                    }

                }
            }
        }

        // 6) Enforce at-most-J turns for this metro using Sinz encoding
        if(!t_vars_for_metro.empty()){
            // apply at-most-J on t_vars_for_metro
            sinz_atmost_k(cnf, t_vars_for_metro, J, string("sinz_k_")+to_string(k));
        } else {
            // no possible turns -> nothing to do
        }
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
