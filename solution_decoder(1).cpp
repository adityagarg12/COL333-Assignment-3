#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <set>

using namespace std;

// Structure to hold city details
struct City {
    int scenario;
    int N, M, K, J, P;
    vector<pair<int, int>> starts;
    vector<pair<int, int>> ends;
};

// Variable encoding: X(k, r, c, d_in, j) is true if metro k is at cell (r,c),
// having arrived from direction d_in with j turns remaining.
// d_in: 0:Left, 1:Right, 2:Up, 3:Down, 4:Start
int get_var(int k, int r, int c, int d_in, int j, const City& city) {
    if (k < 0 || k >= city.K || r < 0 || r >= city.N || c < 0 || c >= city.M || d_in < 0 || d_in > 4 || j < 0 || j > city.J) {
        return 0; // Invalid variable
    }
    int M = city.M;
    int J = city.J;
    // K * N * M * 5_directions * (J+1)_turns
    return 1 + j + d_in * (J + 1) + c * 5 * (J + 1) + r * M * 5 * (J + 1) + k * city.N * M * 5 * (J + 1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./solution_decoder <basename>" << endl;
        return 1;
    }

    string basename = argv[1];
    string city_file = basename + ".city";
    string sat_output_file = basename + ".satoutput";
    string metromap_file = basename + ".metromap";

    ifstream city_in(city_file);
    City city;
    city_in >> city.scenario;
    if (city.scenario == 1) {
        city_in >> city.N >> city.M >> city.K >> city.J;
        city.P = 0;
    } else {
        city_in >> city.N >> city.M >> city.K >> city.J >> city.P;
    }

    for (int i = 0; i < city.K; ++i) {
        int sx, sy, ex, ey;
        city_in >> sx >> sy >> ex >> ey;
        city.starts.push_back({sx, sy});
        city.ends.push_back({ex, ey});
    }
    city_in.close();

    ifstream sat_in(sat_output_file);
    string status;
    sat_in >> status;

    ofstream map_out(metromap_file);

    if (status == "UNSAT") {
        map_out << "0" << endl;
        sat_in.close();
        map_out.close();
        return 0;
    }

    set<int> true_vars;
    int var;
    while (sat_in >> var && var != 0) {
        if (var > 0) {
            true_vars.insert(var);
        }
    }
    sat_in.close();

    for (int k = 0; k < city.K; ++k) {
        string path_str = "";
        pair<int, int> current_pos = city.starts[k];
        int current_d_in = 4; // Start
        int current_j = city.J;

        while (current_pos != city.ends[k]) {
            // Find the next move from the current state
            bool found_move = false;
            
            // Outgoing directions: R, L, U, D
            int dr[] = {0, 0, -1, 1};
            int dc[] = {1, -1, 0, 0};
            char move_char[] = {'R', 'L', 'U', 'D'};
            int opposite_dir[] = {0, 1, 2, 3}; // L->R, R->L, U->D, D->U

            for (int d_out = 0; d_out < 4; ++d_out) {
                int nr = current_pos.first + dr[d_out];
                int nc = current_pos.second + dc[d_out];
                
                if (nr < 0 || nr >= city.N || nc < 0 || nc >= city.M) continue;

                bool is_turn = (current_d_in != 4) && (current_d_in / 2 != d_out / 2);
                int j_new = current_j - (is_turn ? 1 : 0);

                if (j_new >= 0) {
                    int next_var = get_var(k, nr, nc, opposite_dir[d_out], j_new, city);
                    if (true_vars.count(next_var)) {
                        path_str += move_char[d_out];
                        current_pos = {nr, nc};
                        current_d_in = opposite_dir[d_out];
                        current_j = j_new;
                        found_move = true;
                        break;
                    }
                }
            }
            if (!found_move) {
                // Should not happen in a valid SAT solution
                break;
            }
        }
        map_out << path_str << "0" << endl;
    }

    map_out.close();
    return 0;
}
