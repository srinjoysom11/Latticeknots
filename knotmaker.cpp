#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <tuple>
#include <memory>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Constants
const int MAX_LEN = 14;
const int GRID_OFFSET = 32;
const int GRID_SIZE = 64;
const int NUM_THREADS = 6;

// Directions: n, s, e, w, u, d 
const int DX[6] = {0, 0, 1, -1, 0, 0};
const int DY[6] = {1, -1, 0, 0, 0, 0};
const int DZ[6] = {0, 0, 0, 0, 1, -1};
const char DIR_CHARS[6] = {'n', 's', 'e', 'w', 'u', 'd'};
const int INV[6] = {1, 0, 3, 2, 5, 4};

int char_to_int(char c) {
    switch(c) {
        case 'n': return 0;
        case 's': return 1;
        case 'e': return 2;
        case 'w': return 3;
        case 'u': return 4;
        case 'd': return 5;
    }
    return 0;
}

// Fast Grid for SAP check
struct FastGrid {
    bool grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
    vector<tuple<int,int,int>> history;

    FastGrid() {
        memset(grid, 0, sizeof(grid));
        history.reserve(MAX_LEN);
    }

    inline void set(int x, int y, int z) {
        int nx = x + GRID_OFFSET;
        int ny = y + GRID_OFFSET;
        int nz = z + GRID_OFFSET;
        if (!grid[nx][ny][nz]) {
            grid[nx][ny][nz] = true;
            history.emplace_back(nx, ny, nz);
        }
    }

    inline bool is_set(int x, int y, int z) const {
        return grid[x + GRID_OFFSET][y + GRID_OFFSET][z + GRID_OFFSET];
    }

    void clear() {
        for (const auto& p : history) {
            grid[get<0>(p)][get<1>(p)][get<2>(p)] = false;
        }
        history.clear();
    }
};

// Check SAP using FastGrid
bool check_sap(const string& word) {
    static thread_local unique_ptr<FastGrid> grid_ptr;
    if (!grid_ptr) grid_ptr = make_unique<FastGrid>();
    FastGrid& grid = *grid_ptr;
    
    grid.clear();
    
    int x = 0, y = 0, z = 0;
    grid.set(0, 0, 0);
    
    int n = word.length();
    for (int i = 0; i < n - 1; ++i) {
        int dir = char_to_int(word[i]);
        x += DX[dir];
        y += DY[dir];
        z += DZ[dir];
        
        if (grid.is_set(x, y, z)) return false;
        grid.set(x, y, z);
    }
    
    // Last step must return to origin
    int dir = char_to_int(word[n-1]);
    x += DX[dir];
    y += DY[dir];
    z += DZ[dir];
    
    return (x == 0 && y == 0 && z == 0);
}

// Specialized DFS for base case
void simple_dfs(int target_len, string& current, int x, int y, int z, bool visited[64][64][64], vector<string>& out) {
    if (current.length() == target_len) {
        if (x == 0 && y == 0 && z == 0) out.push_back(current);
        return;
    }
    
    if (abs(x) + abs(y) + abs(z) > target_len - current.length()) return;

    for (int i = 0; i < 6; ++i) {
        if (!current.empty() && i == INV[char_to_int(current.back())]) continue;
        
        int nx = x + DX[i];
        int ny = y + DY[i];
        int nz = z + DZ[i];
        
        bool is_origin = (nx == 0 && ny == 0 && nz == 0);
        if (is_origin) {
            if (current.length() + 1 < target_len) continue;
        } else {
            if (visited[nx+32][ny+32][nz+32]) continue;
        }
        
        if (!is_origin) visited[nx+32][ny+32][nz+32] = true;
        current += DIR_CHARS[i];
        simple_dfs(target_len, current, nx, ny, nz, visited, out);
        current.pop_back();
        if (!is_origin) visited[nx+32][ny+32][nz+32] = false;
    }
}

// Worker
void expand_worker(vector<string> input_chunk, int thread_id) {
    // Single output file for this thread
    string fname = "out_thread_" + to_string(thread_id) + ".txt";
    ofstream outfile(fname);
    if (!outfile.is_open()) {
        cerr << "Error: Thread " << thread_id << " failed to open " << fname << endl;
        return;
    }
    
    // Local buffer to minimize I/O calls
    vector<string> buffer;
    buffer.reserve(10000);

    for (const string& word : input_chunk) {
        int n = word.length();
        for (int i = 0; i <= n; ++i) {
            for (int j = i + 1; j <= n; ++j) { // i != j
                for (int dir = 0; dir < 6; ++dir) {
                    char d_char = DIR_CHARS[dir];
                    char inv_char = DIR_CHARS[INV[dir]];
                    
                    // Pre-check i
                    bool bad = false;
                    if (i > 0 && char_to_int(word[i-1]) == INV[dir]) bad = true;
                    if (!bad && i < n && char_to_int(word[i]) == INV[dir]) bad = true;
                    if (bad) continue;

                    // Check inv at j+1
                    if (j > 0 && char_to_int(word[j-1]) == dir) bad = true; 
                    if (!bad && j < n && char_to_int(word[j]) == dir) bad = true;
                    if (bad) continue;

                    // Construct
                    string candidate;
                    candidate.reserve(n + 2);
                    candidate.append(word, 0, i);
                    candidate.push_back(d_char);
                    candidate.append(word, i, j - i);
                    candidate.push_back(inv_char);
                    candidate.append(word, j, n - j);
                    
                    if (check_sap(candidate)) {
                        buffer.push_back(candidate);
                        if (buffer.size() >= 10000) {
                            for(const auto& s : buffer) outfile << s << "\n";
                            buffer.clear();
                        }
                    }
                }
            }
        }
    }
    
    // Flush
    if (!buffer.empty()) {
        for(const auto& s : buffer) outfile << s << "\n";
    }
    outfile.close();
}

int main() {
    // Clean up old files
    for (const auto& entry : fs::directory_iterator(".")) {
        string p = entry.path().string();
        if (p.find("out_thread_") != string::npos || p.find("knots_") != string::npos) {
            fs::remove(entry.path());
        }
    }

    ofstream results_file("knot_counts.csv");
    results_file << "Length,Count\n";
    
    // Length 4
    cout << "Generating Length 4..." << endl;
    vector<string> base_knots;
    bool visited[64][64][64] = {0};
    visited[32][32][32] = true;
    string current = "";
    simple_dfs(4, current, 0, 0, 0, visited, base_knots);
    
    cout << "Length 4: " << base_knots.size() << endl;
    results_file << "4," << base_knots.size() << endl;
    
    // Write to file
    {
        ofstream out("knots_4.txt");
        for(const auto& s : base_knots) out << s << "\n";
    }
    
    // DP
    for (int len = 4; len < 16; len += 2) {
        int next_len = len + 2;
        cout << "Processing Length " << len << " -> " << next_len << endl;
        
        // Read input
        vector<string> inputs;
        {
            ifstream in("knots_" + to_string(len) + ".txt");
            string line;
            while(getline(in, line)) inputs.push_back(line);
        }
        
        // Parallel Expand: split by starting direction (n,s,e,w,u,d)
        vector<vector<string>> inputs_by_dir(6);
        for (const auto &w : inputs) {
            if (w.empty()) continue;
            int d = char_to_int(w[0]);
            if (d < 0 || d >= 6) continue;
            inputs_by_dir[d].push_back(w);
        }

        vector<thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            if (inputs_by_dir[t].empty()) continue;
            threads.emplace_back(expand_worker, move(inputs_by_dir[t]), t);
        }

        for (auto &th : threads) th.join();
        
        // Consolidate
        cout << "Consolidating..." << endl;
        unordered_set<string> unique_knots;
        for(int t=0; t<NUM_THREADS; ++t) {
            string fname = "out_thread_" + to_string(t) + ".txt";
            if (!fs::exists(fname)) continue;
            ifstream in(fname);
            string s;
            while(in >> s) unique_knots.insert(s);
            in.close();
            fs::remove(fname);
        }
        
        ofstream next_file("knots_" + to_string(next_len) + ".txt");
        for(const auto& s : unique_knots) {
            next_file << s << "\n";
        }
        next_file.close();
        
        size_t total_next = unique_knots.size();
        cout << "Length " << next_len << ": " << total_next << endl;
        results_file << next_len << "," << total_next << endl;
        
        // Delete old file
        fs::remove("knots_" + to_string(len) + ".txt");
    }
    
    return 0;
}
