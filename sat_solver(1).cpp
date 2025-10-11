#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

using namespace std;

// Structure to hold city details
struct City {
    int scenario;
    int N, M, K, J, P;
    vector<pair<int, int>> starts;
    vector<pair<int, int>> ends;
    vector<pair<int, int>> popular;
};

// Variable encoding: X(k, r, c, d_in, j) is true if metro k is at cell (r,c),
// having arrived from direction d_in with j turns remaining.
// d_in: 0:Left, 1:Right, 2:Up, 3:Down, 4:Start





int get_state_var(int k, int r, int c, int d_in, int j, const City& city) {
    if (k < 0 || k >= city.K || r < 0 || r >= city.N || c < 0 || c >= city.M || d_in < 0 || d_in > 4 || j < 0 || j > city.J) {
        return 0; // Invalid variable
    }
    int M = city.M;
    int J = city.J;
    // K * N * M * 5_directions * (J+1)_turns
    return 1 + j + d_in * (J + 1) + c * 5 * (J + 1) + r * M * 5 * (J + 1) + k * city.N * M * 5 * (J + 1);
}

// NEW: Helper variable Occ(k, r, c) for cell occupancy
int get_occupancy_var(int k, int r, int c, const City& city, int state_vars_count) {
    return state_vars_count + 1 + c + r * city.M + k * city.N * city.M;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./sat_solver <basename>" << endl;
        return 1;
    }

    string basename = argv[1];
    string city_file = basename + ".city";
    string sat_input_file = basename + ".satinput";

    ifstream fin(city_file);
    if (!fin.is_open()) {
        cerr << "Error opening city file: " << city_file << endl;
        return 1;
    }

    City city;
    fin >> city.scenario;
    if (city.scenario == 1) {
        fin >> city.N >> city.M >> city.K >> city.J;
        city.P = 0;
    } else {
        fin >> city.N >> city.M >> city.K >> city.J >> city.P;
    }

    for (int i = 0; i < city.K; ++i) {
        int sx, sy, ex, ey;
        fin >> sx >> sy >> ex >> ey;
        city.starts.push_back({sx, sy});
        city.ends.push_back({ex, ey});
    }

    if (city.scenario == 2) {
        for (int i = 0; i < city.P; ++i) {
            int px, py;
            fin >> px >> py;
            city.popular.push_back({px, py});
        }
    }
    fin.close();
    
    vector<vector<int>> clauses;
    int state_vars_count = city.K * city.N * city.M * 5 * (city.J + 1);
    int occupancy_vars_count = city.K * city.N * city.M;
    int total_vars = state_vars_count + occupancy_vars_count;

    // --- Linking state variables to occupancy variables ---
    for (int k = 0; k < city.K; ++k) {
        for (int r = 0; r < city.N; ++r) {
            for (int c = 0; c < city.M; ++c) {
                int occ_var = get_occupancy_var(k, r, c, city, state_vars_count);
                vector<int> states_for_this_cell_clause = {-occ_var};
                for (int d = 0; d <= 4; ++d) {
                    for (int j = 0; j <= city.J; ++j) {
                        int state_var = get_state_var(k, r, c, d, j, city);
                        // Link state -> occupancy: if state_var is true, occ_var must be true
                        clauses.push_back({-state_var, occ_var});
                        states_for_this_cell_clause.push_back(state_var);
                    }
                }
                // Link occupancy -> states: if occ_var is true, at least one state_var must be true
                clauses.push_back(states_for_this_cell_clause);
            }
        }
    }

    // --- Constraint: At most one metro per cell (using occupancy vars) ---
    for (int r = 0; r < city.N; ++r) {
        for (int c = 0; c < city.M; ++c) {
            for (int k1 = 0; k1 < city.K; ++k1) {
                for (int k2 = k1 + 1; k2 < city.K; ++k2) {
                    int occ1 = get_occupancy_var(k1, r, c, city, state_vars_count);
                    int occ2 = get_occupancy_var(k2, r, c, city, state_vars_count);
                    clauses.push_back({-occ1, -occ2});
                }
            }
        }
    }

    // Constraint: A cell, if occupied by a metro, has a unique state
    for (int k = 0; k < city.K; ++k) {
        for (int r = 0; r < city.N; ++r) {
            for (int c = 0; c < city.M; ++c) {
                vector<int> vars;
                 for (int d = 0; d <= 4; ++d) {
                    for (int j = 0; j <= city.J; ++j) {
                        vars.push_back(get_state_var(k,r,c,d,j,city));
                    }
                }
                for (size_t i = 0; i < vars.size(); ++i) {
                    for (size_t l = i + 1; l < vars.size(); ++l) {
                        clauses.push_back({-vars[i], -vars[l]});
                    }
                }
            }
        }
    }

    // Constraints per metro line
    for (int k = 0; k < city.K; ++k) {
        int sr = city.starts[k].first, sc = city.starts[k].second;
        int er = city.ends[k].first, ec = city.ends[k].second;

        // Start node must be in start state with J turns.
        clauses.push_back({get_state_var(k, sr, sc, 4, city.J, city)});
        
        // End node must be occupied.
        clauses.push_back({get_occupancy_var(k,er,ec,city,state_vars_count)});

        // Path continuity
        for (int r = 0; r < city.N; ++r) {
            for (int c = 0; c < city.M; ++c) {
                if (r == er && c == ec) continue; 

                for (int d_in = 0; d_in <= 4; ++d_in) {
                    for (int j = 0; j <= city.J; ++j) {
                        int current_var = get_state_var(k, r, c, d_in, j, city);
                        vector<int> outgoing_clause = {-current_var};
                        
                        int dr[] = {0, 0, -1, 1}; // R, L, U, D
                        int dc[] = {1, -1, 0, 0};
                        int next_d_in[] = {1, 0, 3, 2}; // R->L, L->R, U->D, D->U 
                        
                        for (int d_out = 0; d_out < 4; ++d_out) {
                            int nr = r + dr[d_out];
                            int nc = c + dc[d_out];

                            if(d_in == 0 && d_out == 1) continue; // R->L
                            if(d_in == 1 && d_out == 0) continue; // L->R
                            if(d_in == 2 && d_out == 3) continue; // U->D
                            if(d_in == 3 && d_out == 2) continue; // D->U


                            if (nr < 0 || nr >= city.N || nc < 0 || nc >= city.M) continue;
                            
                            bool is_turn = (d_in != 4) && (d_in / 2 != d_out / 2);
                             if (d_in == 0 || d_in == 1) is_turn = !(d_out == 0 || d_out == 1);
                             if (d_in == 2 || d_in == 3) is_turn = !(d_out == 2 || d_out == 3);
                             if (d_in == 4) is_turn = false;

                            int j_new = j - (is_turn ? 1 : 0);

                            if (j_new >= 0) {
                                outgoing_clause.push_back(get_state_var(k, nr, nc, next_d_in[d_out], j_new, city));
                            }
                        }
                        clauses.push_back(outgoing_clause);
                    }
                }
            }
        }
    }

    // Scenario 2: Popular cells must be visited
    if (city.scenario == 2) {
        for (const auto& p_cell : city.popular) {
            vector<int> popular_clause;
            for (int k = 0; k < city.K; ++k) {
               popular_clause.push_back(get_occupancy_var(k, p_cell.first, p_cell.second, city, state_vars_count));
            }
            if(!popular_clause.empty()) clauses.push_back(popular_clause);
        }
    }

    ofstream fout(sat_input_file);
    if (!fout.is_open()) {
        cerr << "Error opening SAT input file for writing: " << sat_input_file << endl;
        return 1;
    }
    
    fout << "p cnf " << total_vars << " " << clauses.size() << endl;
    for (const auto& clause : clauses) {
        for (int literal : clause) {
            fout << literal << " ";
        }
        fout << "0" << endl;
    }
    fout.close();

    return 0;
}
