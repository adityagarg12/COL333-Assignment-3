#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>

using namespace std;

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(const Point& other) const {
        return x < other.x || (x == other.x && y < other.y);
    }
};

class MetroSATSolver {
private:
    int N, M, K, J, P, scenario;
    vector<Point> starts, ends, popular;
    int var_counter;
    vector<vector<int>> clauses;
    map<vector<int>, int> var_map;

    void init_var(){
        var_counter = 1; // Start variable numbering from 1
        clauses.clear();
        var_map.clear();
        for(int k = 0 ; k < K ; k++){
            for(int x = 0 ; x < N ; x++){  // N columns (x coordinate)
                for(int y = 0 ; y < M ; y++){  // M rows (y coordinate)
                    for(int d = 0 ; d <= 4 ; d++){
                        for(int j = 0 ; j <= J ; j++){
                            vector<int> key = {1, k, x, y, d, j}; // x,y order to match Point(x,y)
                            var_map[key] = var_counter++;
                        }
                    }
                }
            }
        }

        for(int k = 0 ; k < K ; k++){
            for(int x = 0 ; x < N ; x++){  // N columns (x coordinate)
                for(int y = 0 ; y < M ; y++){  // M rows (y coordinate)
                    vector<int> key = {0, k, x, y}; // x,y order to match Point(x,y)
                    var_map[key] = var_counter++;
                }
            }
        }
    }

    int manhattan_distance(const Point& p1, const Point& p2) {
        return abs(p1.x - p2.x) + abs(p1.y - p2.y);
    }

    // int get_var(const vector<int>& key) {
    //     if (var_map.find(key) == var_map.end()) {
    //         var_map[key] = var_counter++;
    //     }
    //     return var_map[key];
    // }

    // Safer get_var for a pre-allocation strategy
    int get_var(const vector<int>& key) {
        auto it = var_map.find(key);
        if (it == var_map.end()) {
            // This indicates a bug in the code's logic, not something to recover from.
            cerr << "FATAL ERROR: A variable key was used that was not pre-allocated." << endl;
            // You could print the key here for debugging.
            exit(1);
        }
        return it->second;
    }

    void add_clause(const vector<int>& literals) {
        if (!literals.empty()) {
            clauses.push_back(literals);
        }
    }

    vector<vector<Point>> generate_valid_paths(const Point& start, const Point& end, int max_turns) {
        vector<vector<Point>> valid_paths;
        queue<tuple<int, int, vector<Point>, int, int>> q; // x, y, path, turns, last_dir
        
        // More permissive visited state - only prevent cycles and excessive exploration
        map<tuple<int, int, int>, int> best_length; // (x, y, turns) -> shortest path length

        vector<Point> directions = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}}; // R, D, L, U

        q.push(make_tuple(start.x, start.y, vector<Point>{start}, 0, -1));

        int explored_states = 0;
        bool debug_this_path = (start.x == 4 && start.y == 9 && end.x == 3 && end.y == 1);

        while (!q.empty() && valid_paths.size() < 1000) { // Increased limit significantly
            auto [x, y, path, turns, last_dir] = q.front();
            q.pop();
            explored_states++;

            if (x == end.x && y == end.y) {
                valid_paths.push_back(path);
                continue;
            }

            // Much more permissive pruning - only prevent very excessive exploration
            auto key = make_tuple(x, y, turns);
            if (best_length.count(key) && best_length[key] + 8 < (int)path.size()) {
                continue; // Only skip if current path is MUCH longer
            }
            best_length[key] = min(best_length.count(key) ? best_length[key] : 999, (int)path.size());

            for (int d = 0; d < 4; d++) {
                int nx = x + directions[d].x;
                int ny = y + directions[d].y;

                // Debug specific moves
                if (debug_this_path && x == 4 && y == 9 && d == 0) { // Moving right from (4,9)
                    cerr << "[PATH_DEBUG] From (4,9) trying to move RIGHT to (" << nx << "," << ny << ")" << endl;
                    cerr << "  Bounds check: nx=" << nx << " < N=" << N << " ? " << (nx < N) << endl;
                    cerr << "  Bounds check: ny=" << ny << " < M=" << M << " ? " << (ny < M) << endl;
                }

                if (nx < 0 || nx >= N || ny < 0 || ny >= M) {
                    if (debug_this_path && x == 4 && y == 9) {
                        cerr << "  REJECTED: Out of bounds" << endl;
                    }
                    continue;
                }

                // Check if already in path
                bool in_path = false;
                for (const auto& p : path) {
                    if (p.x == nx && p.y == ny) {
                        in_path = true;
                        break;
                    }
                }
                if (in_path) continue;

                int new_turns = turns;
                if (last_dir != -1 && d != last_dir) {
                    new_turns++;
                }

                if (new_turns > max_turns) continue;

                // More permissive length heuristic 
                int max_reasonable_length = manhattan_distance(start, end) + 4 * max_turns + 5;
                if ((int)path.size() > max_reasonable_length) continue;

                vector<Point> new_path = path;
                new_path.push_back(Point(nx, ny));
                q.push(make_tuple(nx, ny, new_path, new_turns, d));
                
                if (debug_this_path && x == 4 && y == 9 && d == 0) {
                    cerr << "  ACCEPTED: Added (" << nx << "," << ny << ") to queue" << endl;
                }
            }
        }

        return valid_paths;
    }

    void encode_metro_line(int k) {
        Point start = starts[k];
        Point end = ends[k];

        // Metro must use start and end cells
        add_clause({get_var({1, k, start.x, start.y, 4, J})});  // path[k][start.x][start.y]
        // add_clause({get_var({0, k, start.x, start.y})});  // cell[k][start.x][start.y]
        add_clause({get_var({0, k, end.x, end.y})});      // cell[k][end.x][end.y]

        // End cell constraint: no outgoing transitions from end cell
        // The end cell can only be in states, but cannot transition to other cells
        // This is automatically handled by the transition logic since we exclude end cell
        // from the main transition loop below

        // vector<vector<Point>> valid_paths = generate_valid_paths(start, end, J);

        // if (valid_paths.empty()) {
        //     add_clause({}); // Empty clause = unsatisfiable
        //     return;
        // }

        // vector<int> path_vars;
        // for (int i = 0; i < (int)valid_paths.size(); i++) {
        //     int path_var = get_var({1, k, i}); // path[k][i]
        //     path_vars.push_back(path_var);

        //     // If this path is selected, all its cells must be used
        //     for (const Point& p : valid_paths[i]) {
        //         int cell_var = get_var({0, k, p.x, p.y});
        //         add_clause({-path_var, cell_var});
        //     }
        // }
        // Exactly one path must be selected
        // add_clause(path_vars); // At least one
        // for (int i = 0; i < (int)path_vars.size(); i++) {
        //     for (int j = i + 1; j < (int)path_vars.size(); j++) {
        //         add_clause({-path_vars[i], -path_vars[j]}); // At most one
        //     }
        // }

        // // If a cell is used, it must be part of the selected path
        // for (int x = 0; x < N; x++) {
        //     for (int y = 0; y < M; y++) {
        //         int cell_var = get_var({0, k, x, y});

        //         vector<int> containing_paths;
        //         for (int i = 0; i < (int)valid_paths.size(); i++) {
        //             for (const Point& p : valid_paths[i]) {
        //                 if (p.x == x && p.y == y) {
        //                     containing_paths.push_back(get_var({1, k, i}));
        //                     break;
        //                 }
        //             }
        //         }

        //         if (!containing_paths.empty()) {
        //             vector<int> clause = {-cell_var};
        //             clause.insert(clause.end(), containing_paths.begin(), containing_paths.end());
        //             add_clause(clause);
        //         } else {
        //             add_clause({-cell_var});
        //         }
        //     }
        // }


        for (int x = 0; x < N; ++x) {  // N columns
            for (int y = 0; y < M; ++y) {  // M rows
                // End cell should be a sink: no outgoing transitions, but can receive incoming
                
                for (int d_in = 0; d_in <= 4; ++d_in) {
                    for (int j = 0; j <= J; ++j) {
                        int current_var = get_var({1, k, x, y, d_in, j}); // x,y order
                        
                        // Forward flow: if in current state, must go somewhere (EXCEPT for end cell)
                        if (x != end.x || y != end.y) {
                            vector<int> outgoing_clause = {-current_var};
                            
                            int dx[] = {1, -1, 0, 0}; // R, L, U, D (x direction)
                            int dy[] = {0, 0, -1, 1}; // R, L, U, D (y direction)
                            int next_d_in[] = {1, 0, 3, 2}; // R->L, L->R, U->D, D->U 
                            
                            for (int d_out = 0; d_out < 4; ++d_out) {
                                int nx = x + dx[d_out]; // New x (column)
                                int ny = y + dy[d_out]; // New y (row)

                                if (nx < 0 || nx >= N || ny < 0 || ny >= M) continue; // Bounds check
                                
                                // Disallow immediate 180-degree reversal: don't go back the way we came
                                // d_in: direction we came FROM (0=R, 1=L, 2=U, 3=D, 4=start)
                                // d_out: direction we're going TO (0=R, 1=L, 2=U, 3=D)
                                // Opposite direction pairs: R(0)↔L(1), U(2)↔D(3)
                                if (d_in != 4) {
                                    int opposite_direction;
                                    if (d_in == 0) opposite_direction = 1;      // R ↔ L
                                    else if (d_in == 1) opposite_direction = 0; // L ↔ R  
                                    else if (d_in == 2) opposite_direction = 3; // U ↔ D
                                    else if (d_in == 3) opposite_direction = 2; // D ↔ U
                                    else opposite_direction = -1;
                                    
                                    if (d_out == opposite_direction) {
                                        continue; // Don't go back the way we came
                                    }
                                }

                                bool is_turn = false;
                                if (d_in == 4) {
                                    is_turn = false;
                                } else {
                                    // A turn occurs when switching between horizontal (0,1) and vertical (2,3)
                                    is_turn = (d_in / 2 != d_out / 2);
                                }

                                int j_new = j - (is_turn ? 1 : 0);

                                if (j_new >= 0) {
                                    outgoing_clause.push_back(get_var({1, k, nx, ny, next_d_in[d_out], j_new}));
                                }
                            }
                            // Add forward flow constraint (only for non-end cells)
                            clauses.push_back(outgoing_clause);
                        }
                    }
                }
            }
        }

        // Add reverse flow constraints: if in a state (not start), must have come from somewhere
        for (int x = 0; x < N; ++x) {  // N columns
            for (int y = 0; y < M; ++y) {  // M rows
                // Skip start cell - it doesn't need incoming flow
                if (x == start.x && y == start.y) continue;

                for (int d_in = 0; d_in <= 4; ++d_in) {
                    for (int j = 0; j <= J; ++j) {
                        // Skip start state d_in=4 for non-start cells
                        if (d_in == 4) continue;
                        
                        int current_var = get_var({1, k, x, y, d_in, j});
                        vector<int> incoming_clause = {-current_var};
                        
                        int dx[] = {1, -1, 0, 0}; // R, L, U, D (x direction)
                        int dy[] = {0, 0, -1, 1}; // R, L, U, D (y direction)
                        
                        // Find all possible previous states that could lead to current state
                        for (int d_prev_out = 0; d_prev_out < 4; ++d_prev_out) {
                            // Calculate previous cell position
                            int prev_x = x - dx[d_prev_out]; // Previous x (column)
                            int prev_y = y - dy[d_prev_out]; // Previous y (row)
                            
                            if (prev_x < 0 || prev_x >= N || prev_y < 0 || prev_y >= M) continue;
                            
                            // Check if d_prev_out leads to our d_in
                            int expected_d_in[] = {1, 0, 3, 2}; // R->L, L->R, U->D, D->U
                            if (expected_d_in[d_prev_out] != d_in) continue;
                            
                            // Calculate possible previous states
                            for (int prev_d_in = 0; prev_d_in <= 4; ++prev_d_in) {
                                // Check turn logic
                                bool is_turn = false;
                                if (prev_d_in == 4) {
                                    is_turn = false;
                                } else {
                                    is_turn = (prev_d_in / 2 != d_prev_out / 2);
                                }
                                
                                int prev_j = j + (is_turn ? 1 : 0);
                                if (prev_j > J) continue;
                                
                                // Check 180-degree reversal prevention
                                if (prev_d_in != 4) {
                                    int opposite_direction;
                                    if (prev_d_in == 0) opposite_direction = 1;      // R ↔ L
                                    else if (prev_d_in == 1) opposite_direction = 0; // L ↔ R  
                                    else if (prev_d_in == 2) opposite_direction = 3; // U ↔ D
                                    else if (prev_d_in == 3) opposite_direction = 2; // D ↔ U
                                    else opposite_direction = -1;
                                    
                                    if (d_prev_out == opposite_direction) {
                                        continue; // This transition would be blocked by reversal prevention
                                    }
                                }
                                
                                incoming_clause.push_back(get_var({1, k, prev_x, prev_y, prev_d_in, prev_j}));
                            }
                        }
                        
                        // Reverse flow: if in current state, must have come from valid previous state
                        if (incoming_clause.size() > 1) { // Only add if there are valid incoming states
                            clauses.push_back(incoming_clause);
                        }
                        else if (x != start.x || y != start.y) {
                            clauses.push_back({-current_var});
                        }
                    }
                }
            }
        }






    }

public:
    MetroSATSolver() : var_counter(1) {}

    void read_input(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << endl;
            exit(1);
        }

        string line;
        vector<string> lines;
        while (getline(file, line)) {
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        scenario = stoi(lines[0]);

        istringstream iss(lines[1]);
        if (scenario == 1) {
            iss >> N >> M >> K >> J;
            P = 0;
        } else {
            iss >> N >> M >> K >> J >> P;
        }

        for (int i = 0; i < K; i++) {
            istringstream line_stream(lines[2 + i]);
            int sx, sy, ex, ey;
            line_stream >> sx >> sy >> ex >> ey;
            starts.push_back(Point(sx, sy));
            ends.push_back(Point(ex, ey));
        }

        if (scenario == 2) {
            istringstream pop_stream(lines[2 + K]);
            int coord;
            vector<int> coords;
            while (pop_stream >> coord) {
                coords.push_back(coord);
            }
            for (int i = 0; i < (int)coords.size(); i += 2) {
                popular.push_back(Point(coords[i], coords[i + 1]));
            }
        }
    }

    void encode_problem() {
        cout << "Encoding: " << N << "x" << M << ", " << K << " metros, max " << J << " turns" << endl;
        init_var();
        for (int k = 0; k < K; k++) {
            encode_metro_line(k);
        }


        // --- Linking state variables to occupancy variables ---
        for (int k = 0; k < K; ++k) {
            for (int x = 0; x < N; ++x) {  // N columns
                for (int y = 0; y < M; ++y) {  // M rows
                    int occ_var = get_var({0, k, x, y}); // x,y order
                    vector<int> states_for_this_cell_clause = {-occ_var};
                    for (int d = 0; d <= 4; ++d) {
                        for (int j = 0; j <= J; ++j) {
                            int state_var = get_var({1, k, x, y, d, j}); // x,y order
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





        // Uniqueness: each cell used by at most one metro
        for (int x = 0; x < N; x++) {
            for (int y = 0; y < M; y++) {
                for (int k1 = 0; k1 < K; k1++) {
                    int cell1 = get_var({0, k1, x, y});
                    for (int k2 = k1 + 1; k2 < K; k2++) {                     
                        int cell2 = get_var({0, k2, x, y});
                        add_clause({-cell1, -cell2});
                    }
                }
            }
        }

        // Popular cells constraint
        if (scenario == 2) {
            for (const Point& p : popular) {
                vector<int> popular_users;
                for (int k = 0; k < K; k++) {
                    popular_users.push_back(get_var({0, k, p.x, p.y}));
                }
                add_clause(popular_users);
            }
        }

        

        // Constraint: A cell, if occupied by a metro, has a unique state
        for (int k = 0; k < K; ++k) {
            for (int x = 0; x < N; ++x) {  // N columns
                for (int y = 0; y < M; ++y) {  // M rows
                    vector<int> vars;
                    for (int d = 0; d <= 4; ++d) {
                        for (int j = 0; j <= J; ++j) {
                            vars.push_back(get_var({1, k, x, y, d, j})); // x,y order
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
    }

    void write_cnf(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot create file " << filename << endl;
            exit(1);
        }

        file << "p cnf " << var_counter - 1 << " " << clauses.size() << endl;
        for (const auto& clause : clauses) {
            for (int lit : clause) {
                file << lit << " ";
            }
            file << "0" << endl;
        }

        cout << "Generated " << clauses.size() << " clauses with " 
             << var_counter - 1 << " variables" << endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input.city>" << endl;
        return 1;
    }

    string input_file = argv[1];
    string output_file = input_file;
    size_t pos = output_file.find(".city");
    if (pos != string::npos) {
        output_file.replace(pos, 5, ".satinput");
    } else {
        output_file += ".satinput";
    }

    MetroSATSolver solver;
    solver.read_input(input_file);

    solver.encode_problem();
    solver.write_cnf(output_file);

    return 0;
}
