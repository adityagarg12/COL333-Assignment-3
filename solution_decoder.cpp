#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <set>
#include <map>
#include <algorithm>

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

class MetroSolutionDecoder {
private:
    int N, M, K, J, P, scenario;
    vector<Point> starts, ends, popular;
    map<int, bool> assignment;
    map<vector<int>, int> var_map;
    int var_counter;

    int get_var(const vector<int>& key) {
        if (var_map.find(key) == var_map.end()) {
            var_map[key] = var_counter++;
        }
        return var_map[key];
    }

    int manhattan_distance(const Point& p1, const Point& p2) {
        return abs(p1.x - p2.x) + abs(p1.y - p2.y);
    }

    vector<vector<Point>> generate_valid_paths(const Point& start, const Point& end, int max_turns) {
        vector<vector<Point>> valid_paths;
        queue<tuple<int, int, vector<Point>, int, int>> q;
        set<tuple<int, int, int, int, int>> visited;

        vector<Point> directions = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

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

    vector<string> path_to_moves(const vector<Point>& path) {
        vector<string> moves;
        if (path.size() < 2) return moves;

        vector<Point> directions = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
        vector<string> dir_names = {"R", "D", "L", "U"};

        for (int i = 0; i < (int)path.size() - 1; i++) {
            int dx = path[i + 1].x - path[i].x;
            int dy = path[i + 1].y - path[i].y;

            for (int d = 0; d < 4; d++) {
                if (dx == directions[d].x && dy == directions[d].y) {
                    moves.push_back(dir_names[d]);
                    break;
                }
            }
        }

        return moves;
    }

    vector<string> simple_path(const Point& start, const Point& end) {
        vector<string> moves;
        int x = start.x, y = start.y;

        while (x < end.x) {
            moves.push_back("R");
            x++;
        }
        while (x > end.x) {
            moves.push_back("L");
            x--;
        }
        while (y < end.y) {
            moves.push_back("D");
            y++;
        }
        while (y > end.y) {
            moves.push_back("U");
            y--;
        }

        return moves;
    }

    void reconstruct_var_mapping() {
        var_map.clear();
        var_counter = 1; // Reset counter to match sat_solver
        
        // Mimic the exact same variable creation order as sat_solver.cpp
        // This follows the encode_metro_line() function for each metro line k
        
        for (int k = 0; k < K; k++) {
            Point start = starts[k];
            Point end = ends[k];
            
            // Start and end cell variables (created first in sat_solver)
            get_var({0, k, start.x, start.y});
            get_var({0, k, end.x, end.y});
            
            vector<vector<Point>> valid_paths = generate_valid_paths(start, end, J);
            
            if (valid_paths.empty()) {
                continue; // Skip if no valid paths (matches sat_solver behavior)
            }
            
            // Path variables (created in the path loop)
            for (int i = 0; i < (int)valid_paths.size(); i++) {
                get_var({1, k, i}); // path[k][i]
                
                // Cell variables for each point in this path
                for (const Point& p : valid_paths[i]) {
                    get_var({0, k, p.x, p.y});
                }
            }
            
            // Additional cell variables created in the consistency check loop
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < M; y++) {
                    get_var({0, k, x, y}); // This ensures all cell variables are created
                }
            }
        }
        
        // Variables for uniqueness constraints (created in encode_problem)
        for (int x = 0; x < N; x++) {
            for (int y = 0; y < M; y++) {
                for (int k1 = 0; k1 < K; k1++) {
                    get_var({0, k1, x, y});
                    for (int k2 = k1 + 1; k2 < K; k2++) {
                        get_var({0, k2, x, y});
                    }
                }
            }
        }
        
        // Variables for popular cells constraint (Scenario 2)
        if (scenario == 2) {
            for (const Point& p : popular) {
                for (int k = 0; k < K; k++) {
                    get_var({0, k, p.x, p.y});
                }
            }
        }
    }

    vector<string> find_path_for_metro(int k) {
        vector<vector<Point>> valid_paths = generate_valid_paths(starts[k], ends[k], J);

        for (int i = 0; i < (int)valid_paths.size(); i++) {
            int path_var = get_var({1, k, i});
            if (assignment.count(path_var) && assignment[path_var]) {
                return path_to_moves(valid_paths[i]);
            }
        }

        // Fallback to simple path
        return simple_path(starts[k], ends[k]);
    }

public:
    MetroSolutionDecoder() : var_counter(1) {}
    
    void read_city_file(const string& filename) {
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

    bool read_sat_output(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        string line;
        vector<string> lines;
        while (getline(file, line)) {
            lines.push_back(line);
        }

        if (lines.empty()) return false;

        string first_line = lines[0];
        first_line.erase(first_line.find_last_not_of(" \t\r\n") + 1);
        if (first_line == "UNSAT") {
            return false;
        }

        for (const string& line : lines) {
            string trimmed = line;
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            if (trimmed.empty() || trimmed[0] == 'c' || trimmed[0] == 's' || trimmed == "SAT") {
                continue;
            }

            istringstream iss(trimmed);
            string token;
            vector<int> variables;

            while (iss >> token) {
                try {
                    int var = stoi(token);
                    variables.push_back(var);
                } catch (...) {
                    continue;
                }
            }

            if (!variables.empty() && variables.back() == 0) {
                variables.pop_back();
            }

            for (int var : variables) {
                assignment[abs(var)] = var > 0;
            }
            return true;
        }

        return false;
    }

    void generate_output(const string& output_file) {
        reconstruct_var_mapping();

        ofstream file(output_file);
        if (!file.is_open()) {
            cerr << "Error: Cannot create file " << output_file << endl;
            exit(1);
        }

        for (int k = 0; k < K; k++) {
            vector<string> path = find_path_for_metro(k);
            if (!path.empty()) {
                for (int i = 0; i < (int)path.size(); i++) {
                    if (i > 0) file << " ";
                    file << path[i];
                }
                file << " 0" << endl;
            } else {
                file << "0" << endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <base_filename>" << endl;
        return 1;
    }

    string base = argv[1];
    string city_file = base + ".city";
    string sat_output_file = base + ".satoutput";
    string metromap_file = base + ".metromap";

    MetroSolutionDecoder decoder;
    decoder.read_city_file(city_file);

    if (!decoder.read_sat_output(sat_output_file)) {
        ofstream file(metromap_file);
        if (file.is_open()) {
            file << "0" << endl;
        }
        cout << "UNSAT - no solution exists" << endl;
        return 0;
    }

    decoder.generate_output(metromap_file);
    cout << "Solution written to " << metromap_file << endl;

    return 0;
}
