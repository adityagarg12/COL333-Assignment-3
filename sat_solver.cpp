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

    int manhattan_distance(const Point& p1, const Point& p2) {
        return abs(p1.x - p2.x) + abs(p1.y - p2.y);
    }

    int get_var(const vector<int>& key) {
        if (var_map.find(key) == var_map.end()) {
            var_map[key] = var_counter++;
        }
        return var_map[key];
    }

    void add_clause(const vector<int>& literals) {
        if (!literals.empty()) {
            clauses.push_back(literals);
        }
    }

    vector<vector<Point>> generate_valid_paths(const Point& start, const Point& end, int max_turns) {
        vector<vector<Point>> valid_paths;
        queue<tuple<int, int, vector<Point>, int, int>> q; // x, y, path, turns, last_dir
        set<tuple<int, int, int, int, int>> visited; // x, y, turns, last_dir, path_length

        vector<Point> directions = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}}; // R, D, L, U

        q.push(make_tuple(start.x, start.y, vector<Point>{start}, 0, -1));

        while (!q.empty() && valid_paths.size() < 100) {
            auto [x, y, path, turns, last_dir] = q.front();
            q.pop();

            if (x == end.x && y == end.y) {
                valid_paths.push_back(path);
                continue;
            }

            auto state = make_tuple(x, y, turns, last_dir, (int)path.size());
            if (visited.count(state)) continue;
            visited.insert(state);

            for (int d = 0; d < 4; d++) {
                int nx = x + directions[d].x;
                int ny = y + directions[d].y;

                if (nx < 0 || nx >= N || ny < 0 || ny >= M) continue;

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

                if ((int)path.size() > manhattan_distance(start, end) + 2 * max_turns) continue;

                vector<Point> new_path = path;
                new_path.push_back(Point(nx, ny));
                q.push(make_tuple(nx, ny, new_path, new_turns, d));
            }
        }

        return valid_paths;
    }

    void encode_metro_line(int k) {
        Point start = starts[k];
        Point end = ends[k];

        // Metro must use start and end cells
        add_clause({get_var({0, k, start.x, start.y})});  // cell[k][start.x][start.y]
        add_clause({get_var({0, k, end.x, end.y})});      // cell[k][end.x][end.y]

        vector<vector<Point>> valid_paths = generate_valid_paths(start, end, J);

        if (valid_paths.empty()) {
            add_clause({}); // Empty clause = unsatisfiable
            return;
        }

        vector<int> path_vars;
        for (int i = 0; i < (int)valid_paths.size(); i++) {
            int path_var = get_var({1, k, i}); // path[k][i]
            path_vars.push_back(path_var);

            // If this path is selected, all its cells must be used
            for (const Point& p : valid_paths[i]) {
                int cell_var = get_var({0, k, p.x, p.y});
                add_clause({-path_var, cell_var});
            }
        }

        // Exactly one path must be selected
        add_clause(path_vars); // At least one
        for (int i = 0; i < (int)path_vars.size(); i++) {
            for (int j = i + 1; j < (int)path_vars.size(); j++) {
                add_clause({-path_vars[i], -path_vars[j]}); // At most one
            }
        }

        // If a cell is used, it must be part of the selected path
        for (int x = 0; x < N; x++) {
            for (int y = 0; y < M; y++) {
                int cell_var = get_var({0, k, x, y});

                vector<int> containing_paths;
                for (int i = 0; i < (int)valid_paths.size(); i++) {
                    for (const Point& p : valid_paths[i]) {
                        if (p.x == x && p.y == y) {
                            containing_paths.push_back(get_var({1, k, i}));
                            break;
                        }
                    }
                }

                if (!containing_paths.empty()) {
                    vector<int> clause = {-cell_var};
                    clause.insert(clause.end(), containing_paths.begin(), containing_paths.end());
                    add_clause(clause);
                } else {
                    add_clause({-cell_var});
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

        for (int k = 0; k < K; k++) {
            encode_metro_line(k);
        }

        // Uniqueness: each cell used by at most one metro
        for (int x = 0; x < N; x++) {
            for (int y = 0; y < M; y++) {
                for (int k1 = 0; k1 < K; k1++) {
                    for (int k2 = k1 + 1; k2 < K; k2++) {
                        int cell1 = get_var({0, k1, x, y});
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
