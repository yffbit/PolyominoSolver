#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <climits>
#include <chrono>
#include <fstream>
using namespace std;
using namespace std::chrono;

vector<int> split(string& s, int offset=0, string delimiter=": ,") {
    vector<int> ans;
    size_t i = s.find_first_not_of(delimiter, offset);
    while(i != string::npos) {
        size_t j = s.find_first_of(delimiter, i+1);
        ans.emplace_back(stoi(s.substr(i, j-i)));
        i = s.find_first_not_of(delimiter, j);
    }
    return ans;
}

template<class T>
size_t vector_hash(const vector<T>& vec) {
    static hash<T> hash_func;
    size_t seed = vec.size();
    for(const T& v : vec) {
        seed ^= hash_func(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

int popcount(unsigned int x) {
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0f0f0f0f) + ((x >> 4) & 0x0f0f0f0f);
    x = (x & 0x00ff00ff) + ((x >> 8) & 0x00ff00ff);
    return (x & 0x0000ffff) + (x >> 16);
}

#define CURSOR_MOVE_UP(x) printf("\033[%dA", (x))

#define EMPTY 0
#define WALL -1

#define ADJ_U (-1)
#define ADJ_D (1)
#define ADJ_L (-M-2)
#define ADJ_R (M+2)

#define MASK_U 1
#define MASK_D 2
#define MASK_L 4
#define MASK_R 8

#define MIRROR_H 1// bit0:水平镜像
#define MIRROR_V 2// bit1:垂直镜像
#define MIRROR_T 4// bit2:转置

struct Point {
    int row, col;
    Point(int row, int col):row(row), col(col) {}
    Point& operator-=(const Point& other) {
        row -= other.row;
        col -= other.col;
        return *this;
    }
};
struct Piece {
    int id;// 部件类别:1~n
    int type;// 形态:0~7
    string str;// 打印字符串
    vector<int> locs;
    Piece() = default;
    Piece(int id, int type, vector<int>& locs, int len):id(id), type(type), locs(locs), str(len, ' ') {
        // (c[0]-'A'+1)*26^0+(c[1]-'A'+1)*26+...+(c[i]-'A'+1)*26^i+...=id
        for(int i = len-1; id; id /= 26) {
            id--;
            str[i--] = id % 26 + 'A';
        }
    }
};
struct Index {
    int type_idx, square_idx;
    Index(int t_idx, int s_idx):type_idx(t_idx), square_idx(s_idx) {}
};
struct Config {
    int M, N;
    vector<Point> holes;
    vector<vector<Point>> pieces;
    vector<Point> corners;// 每种部件的最小外切矩形的右下角坐标
    vector<int> copies;
};
struct Timer {
    steady_clock::time_point t0;
    Timer():t0(steady_clock::now()) {}
    double now() {
        duration<double> d = duration_cast<duration<double>>(steady_clock::now() - t0);
        return d.count();
    }
};

class Solver {
public:
    Solver(Config& config, ofstream& out_file, int freq=0, bool llp=true, bool info=false):
        M(config.M), N(config.N), CheckIsoFreq(freq), LLP(llp), info(info),
        out_file(out_file), board((M+2)*(N+2), EMPTY), liberties((M+2)*(N+2), 0) {
        pre_time = timer.now();
        // 最外层设置为WALL
        for(int j = -1; j <= N; j++) {
            board[rc2loc(-1, j)] = WALL;
            board[rc2loc(M, j)] = WALL;
        }
        for(int i = 0; i < M; i++) {
            board[rc2loc(i, -1)] = WALL;
            board[rc2loc(i, N)] = WALL;
        }
        // 标记不能覆盖的方格
        for(Point& p : config.holes) board[rc2loc(p.row, p.col)] = WALL;
        // 计算每个方格的自由度
        for(int j = 0; j < N; j++) {
            for(int loc = rc2loc(0,j), end_loc = rc2loc(M,j); loc < end_loc; loc += ADJ_D) {
                liberties[loc] = get_liberties(loc);
            }
        }
        init_str(config.copies.size());
        init_pieces(config);
        init_around_idx();
    }
    void solve() {
        _solve(rc2loc(0, 0));
        pre_time = timer.now();
        print_info(false, true);
    }
private:
    ofstream& out_file;// 输出文件
    Timer timer;
    double pre_time;
    bool LLP;// 是否优先求解自由度最小的方块
    bool info;// 是否显示求解过程
    int CheckIsoFreq;
    int check_cnt = 0;// 计数器
    int M, N;// 拼板的行数列数
    int symmetry;// 拼板对称性
    int min_piece = INT_MAX, max_piece = INT_MIN;// 部件方块数的最小值和最大值
    int total_cnt = 0;// 部件(包括副本)总数
    int piece_cnt = 0;// 已经放置的部件个数
    long long trial_cnt = 0;// 放置次数,只增不减
    long long solution_cnt = 0;// 找到多少个解
    long long unique_solution_cnt = 0;// 找到多少个不同解
    int len, str_N;
    string board_str;// 打印字符串
    vector<int> board, new_board;// 每个位置的颜色
    unordered_set<size_t> board_set;// 解去重
    vector<int> min_square_cnt;// 栈,求解过程中剩余部件中的最小方块数
    unordered_map<int, int> square_cnt_map;
    vector<short> liberties;// 每个位置的自由度
    vector<int> copies;
    vector<vector<Piece>> pieces;
    vector<vector<vector<Index>>> around_idx;

    inline int rc2loc(int row, int col) { return (row+1) + (col+1)*(M+2); }// 一列一列的顺序
    inline int loc2row(int loc) { return loc % (M+2) - 1; }// 行坐标
    inline int loc2col(int loc) { return loc / (M+2) - 1; }// 列坐标
    inline int str_loc(int row, int col) { return row * str_N + col * (len+1); }// 一行一行的顺序
    short get_liberties(int loc) {
        short num = 0;
        if(board[loc+ADJ_U] == EMPTY) num++;
        if(board[loc+ADJ_D] == EMPTY) num++;
        if(board[loc+ADJ_L] == EMPTY) num++;
        if(board[loc+ADJ_R] == EMPTY) num++;
        return num;
    }
    int get_around_mask(int loc) {
        int mask = 0;
        if(board[loc+ADJ_U] != EMPTY) mask |= MASK_U;
        if(board[loc+ADJ_D] != EMPTY) mask |= MASK_D;
        if(board[loc+ADJ_L] != EMPTY) mask |= MASK_L;
        if(board[loc+ADJ_R] != EMPTY) mask |= MASK_R;
        return mask;
    }
    void init_around_idx() {// 给定空格子的四周的覆盖情况,判断每种部件每种变换形态中每个方块能否覆盖该空格子
        // 每次填充的目标空格子要么是拼板中剩余的第一个空格子,要么是自由度最小的空格子
        // 由于拼板的外围增加了一圈WALL,所以剩余的第一个空格子的周围至少有两个格子(左边和上边)被WALL或者其他部件覆盖
        // 自由度最小的空格子的周围也至少有两个格子被覆盖
        // 所以只需要考虑周围至少有两个格子被覆盖的情况
        int m = copies.size();
        around_idx = vector<vector<vector<Index>>>(m,vector<vector<Index>>(16));
        for(int i = 0; i < m; i++) {// 每种部件
            int n = pieces[i].size();// 变换形态数
            vector<vector<Index>>& around = around_idx[i];
            for(int j = 0; j < n; j++) {// 每种变换形态
                vector<int>& locs = pieces[i][j].locs;
                int t = locs.size();// 方块个数
                if(t == 1) {
                    for(int s = 0; s < 16; s++) around[s].emplace_back(j, 0);
                    continue;
                }
                unordered_set<int> loc_set(locs.begin(), locs.end());
                for(int k = 0; k < t; k++) {// 每个方块
                    int loc = locs[k];
                    around[0].emplace_back(j, k);
                    for(int s = 1; s < 16; s++) {
                        if((s & MASK_U) && loc_set.find(loc+ADJ_U) != loc_set.end()) continue;
                        if((s & MASK_D) && loc_set.find(loc+ADJ_D) != loc_set.end()) continue;
                        if((s & MASK_L) && loc_set.find(loc+ADJ_L) != loc_set.end()) continue;
                        if((s & MASK_R) && loc_set.find(loc+ADJ_R) != loc_set.end()) continue;
                        around[s].emplace_back(j, k);
                    }
                }
            }
        }
    }
    int board_symmetry() {// 二进制低8位,每一位表示拼板的变换形态和初始形态是否一致
        int T = M == N ? 8 : 4;// 行列不相等时不能转置,按照尺寸(M,N)搜索到的解覆盖的尺寸为(M,N),与尺寸(N,M)不兼容
        int mask = 0;
        Point rd(M-1, N-1);
        for(int type = 1; type < T; type++) mask |= 1 << type;
        for(int i = 0; i < M; i++) {
            for(int j = 0; j < N; j++) {
                int loc = rc2loc(i, j);
                if(board[loc] != WALL) continue;
                for(int type = 1; type < T; type++) {
                    int row = i, col = j;
                    _transform(row, col, rd, type);
                    if(board[loc] != board[rc2loc(row, col)]) mask &= ~(1 << type);
                }
            }
        }
        return mask;
    }
    bool piece_str(vector<Point>& src, Point& rd, unordered_set<string>& piece_set) {// 部件变换形态去重
        int n = rd.col + 1;
        string str = to_string(rd.row+1) + to_string(n);
        int offset = str.size();
        str.insert(str.end(), (rd.row+1)*n, '0');
        for(Point& p : src) str[offset+p.row*n+p.col] = '1';
        if(piece_set.find(str) != piece_set.end()) return true;// 重复
        piece_set.insert(str);
        return false;
    }
    void init_str(int n) {
        len = 1;
        for(int t = 26; t < n; t *= 26) len++;// 确定字符串需要几位
        str_N = N * (len + 1);
        board_str = string(M*str_N, ' ');
        for(int i = str_N-1; i < board_str.size(); i += str_N) board_str[i] = '\n';
        for(int i = 0; i < M; i++) {
            for(int j = 0; j < N; j++) {
                char c = board[rc2loc(i, j)] == EMPTY ? '.' : '#';
                board_str.replace(str_loc(i, j), len, len, c);
            }
        }
    }
    void print_info(bool found=false, bool stop=false) {
        static bool move_cursor = false;
        if(move_cursor) {
            if(info) CURSOR_MOVE_UP(M+1);
            CURSOR_MOVE_UP(4);
        }
        double t = max(pre_time, 0.001);
        printf("%s : Elapsed time %.1f seconds\n", stop?"Completed":"Running", t);
        printf("Number of solutions %lld\n", solution_cnt);
        printf("Number of unique solutions %lld\n", unique_solution_cnt);
        printf("Tried %lld pieces at %.2f per second\n", trial_cnt, trial_cnt/t);
        if(!stop && (found || info)) printf("%s\n", board_str.c_str());
        move_cursor = !found;// 控制下一次输出
    }
    inline void save_solution() {
        out_file << "solution " << unique_solution_cnt << ":\n" << board_str << endl;
    }
    void init_pieces(Config& config) {
        copies = config.copies;
        symmetry = board_symmetry();
        total_cnt = accumulate(copies.begin(), copies.end(), 0);
        int n = copies.size(), i = 0;
        pieces.resize(n);
        vector<pair<int, int>> bit_cnts;
        for(vector<Point>& vec : config.pieces) {
            square_cnt_map[vec.size()] += copies[i];
            min_piece = min(min_piece, (int)vec.size());
            max_piece = max(max_piece, (int)vec.size());
            vector<int> locs;
            convert(vec, locs);
            pieces[i].emplace_back(i+1, 0, locs, len);
            unordered_set<string> piece_set;
            piece_str(vec, config.corners[i], piece_set);
            // 固定拼板的形态,在部件不出界的条件下,部件能以8种变换中的任意1种形态放置在拼板中
            // 如果部件的8种变换形态之间有相同的,则需要去重
            int mask = 0;// 二进制位表示是否保留每种变换形态
            vector<Point> temp;
            for(int type = 1; type < 8; type++) {
                Point rd = config.corners[i];
                if(!transform(vec, rd, type, temp)) continue;
                if(piece_str(temp, rd, piece_set)) continue;
                mask |= 1 << type;
                convert(temp, locs);
                pieces[i].emplace_back(i+1, type, locs, len);
            }
            bit_cnts.emplace_back(popcount(mask & symmetry), -i);
            i++;
        }
        min_square_cnt.resize(total_cnt+1);
        min_square_cnt[0] = min_piece;
        // 选中某种部件,通过减少该种部件的可选变换形态数,过滤一部分重复解
        // 例如,假设选中的部件为A(副本为1),空拼板的水平镜像和初始形态相同,
        // 则部件A的水平镜像对应的解经过水平镜像后就得到了部件A的初始形态的解,
        // 所以可以保留部件A的初始形态,删除其水平镜像形态
        // 如果部件A的副本数大于1,则只能其中某一个的可选变换形态数可以减少,其他的不变
        if(symmetry) {
            sort(bit_cnts.rbegin(), bit_cnts.rend());// 降序排列
            if(bit_cnts[0].first > 0) split_piece(-bit_cnts[0].second);
        }
        // 经过前面两步的去重,求解过程依然会搜索到重复解
        // 例如1*6的拼板,两种部件,1*4,1*2,过滤之后,两种部件都只剩1种变换形态,最终会搜索到2个等价的解
    }
    void split_piece(int i) {
        if(copies[i] > 1) {
            copies.push_back(copies[i]-1);
            copies[i] = 1;
            pieces.push_back(pieces[i]);
        }
        vector<Piece>& piece = pieces[i];
        int n = piece.size(), j = 0;
        while(j < n) {
            if((1<<piece[j].type) & symmetry) {// 删除该变换形态
                piece[j] = move(piece[--n]);
            }
            else j++;
        }
        piece.resize(n);
    }
    int get_min_piece() {
        int cnt = max_piece;
        for(auto it : square_cnt_map) {
            if(it.second && it.first < cnt) cnt = it.first;
        }
        return cnt;
    }
    void convert(vector<Point>& src, vector<int>& dst) {
        dst.clear();
        for(Point& p : src) dst.push_back(rc2loc(p.row, p.col));
        sort(dst.begin(), dst.end());
    }
    void _transform(int& row, int& col, Point& rd, int type) {
        if(type & MIRROR_H) col = rd.col - col;
        if(type & MIRROR_V) row = rd.row - row;
        if(type & MIRROR_T) swap(row, col);
    }
    bool transform(vector<Point>& src, Point& rd, int type, vector<Point>& dst) {
        if(src.empty() || type < 0 || type >= 8) return false;
        dst.clear();
        for(Point& p : src) {
            int i = p.row, j = p.col;
            _transform(i, j, rd, type);
            if(i >= M || j >= N) return false;// 转置后超出拼板范围
            dst.emplace_back(i, j);
        }
        if(type & MIRROR_T) swap(rd.row, rd.col);// 右下角坐标发生变化
        return true;
    }
    void _solve(int loc) {
        loc = piece_cnt == total_cnt ? -1 : find_first_loc(loc, board);
        double curr_time = timer.now();
        if(loc == -1) {
            pre_time = curr_time;
            add_solution();
            return;
        }
        if(curr_time - pre_time >= 2) {// 大约每2s显示一次
            pre_time = curr_time;
            print_info(false);
        }
        int llp = loc, new_loc = loc;
        if(CheckIsoFreq && (++check_cnt) >= CheckIsoFreq) {
            check_cnt = 0;
            if(check_iso_area(new_loc, llp)) return;
        }
        else if(LLP) llp = find_least_liberties(new_loc);
        int n = copies.size(), around_mask = get_around_mask(llp);
        for(int i = 0; i < n; i++) {
            if(!copies[i]) continue;
            vector<Index>& around = around_idx[i][around_mask];
            vector<Piece>& piece = pieces[i];
            for(Index& idx : around) {
                if(llp == new_loc && idx.square_idx) continue;// 只需要尝试每个变换形态的根索引填充llp
                Piece& p = piece[idx.type_idx];
                int offset = llp - p.locs[idx.square_idx];
                if(!fit(p.locs, offset)) continue;
                copies[i]--, piece_cnt++, trial_cnt++;
                min_square_cnt[piece_cnt] = min_square_cnt[piece_cnt-1];
                update_board(p.locs, offset, p.id, p.str);
                _solve(loc);
                update_board(p.locs, offset, EMPTY, p.str);
                copies[i]++, piece_cnt--;
            }
        }
    }
    bool fit(vector<int>& locs, int offset) {
        for(int& loc : locs) {
            if(board[loc+offset] != EMPTY) return false;
        }
        return true;
    }
    void add_solution() {
        solution_cnt++;
        size_t h = vector_hash(board);
        if(board_set.find(h) == board_set.end()) unique_solution_cnt++;
        else {
            print_info(true);
            return;
        }
        print_info(true);
        board_set.insert(h);
        new_board = board;
        Point rd(M-1, N-1);
        for(int type = (M==N)?7:3; type; type--) {
            if(!((1<<type) & symmetry)) continue;
            for(int i = 0; i < M; i++) {
                for(int j = 0; j < N; j++) {
                    int row = i, col = j;
                    _transform(row, col, rd, type);
                    new_board[rc2loc(row, col)] = board[rc2loc(i, j)];
                }
            }
            board_set.insert(vector_hash(new_board));
        }
        save_solution();
    }
    void update_board(vector<int>& locs, int offset, int color, string& str) {// color为EMPTY表示移走部件
        int diff = color == EMPTY ? 1 : -1;
        for(int loc : locs) {
            loc += offset;
            board[loc] = color;
            liberties[loc+ADJ_U] += diff;
            liberties[loc+ADJ_D] += diff;
            liberties[loc+ADJ_L] += diff;
            liberties[loc+ADJ_R] += diff;
            int loc1 = str_loc(loc2row(loc), loc2col(loc));
            if(color == EMPTY) board_str.replace(loc1, len, len, '.');
            else board_str.replace(loc1, len, str);
        }
        square_cnt_map[locs.size()] += diff;
        if(color != EMPTY && piece_cnt < total_cnt && !square_cnt_map[min_square_cnt[piece_cnt-1]]) {// 放置部件并且最小的用完了
            min_square_cnt[piece_cnt] = get_min_piece();
        }
    }
    int find_first_loc(int loc, vector<int>& board) {// 从loc开始找到第一个空格子
        for(int end_loc = rc2loc(M, N-1); loc < end_loc; loc++) {
            if(board[loc] == EMPTY) return loc;
        }
        return -1;
    }
    int find_least_liberties(int loc) {// 剩余空格子中,自由度最小的格子(不区分独立区域)
        int cnt = INT_MAX, llp = -1;
        for(int end_loc = rc2loc(M, N-1); loc < end_loc; loc++) {
            if(board[loc] != EMPTY) continue;
            if(liberties[loc] < cnt) {
                cnt = liberties[loc];
                llp = loc;
                if(cnt == 0) return llp;
            }
        }
        return llp;
    }
    bool check_iso_area(int& loc, int& llp) {// 找最小连通区域的过程中同时搜索llp
        int color = WALL - 1, cnt = INT_MAX, temp_loc = loc, temp_llp;
        int curr_min = min_square_cnt[piece_cnt];
        new_board = board;// 复制
        while(temp_loc != -1) {
            temp_llp = temp_loc;
            int cnt1 = dfs(temp_loc, color, temp_llp);
            if(cnt1 < curr_min) return true;
            if(cnt1 < cnt) {// 找到更小的连通区域
                cnt = cnt1;
                loc = temp_loc;
                llp = temp_llp;
            }
            color--;
            temp_loc = find_first_loc(temp_loc, new_board);
        }
        return false;
    }
    int dfs(int loc, int& color, int& llp) {// 连通区域方格个数,同时求出该区域内自由度最小的格子
        if(new_board[loc] != EMPTY) return 0;
        if(LLP && liberties[loc] < liberties[llp]) llp = loc;
        int ans = 1;
        new_board[loc] = color;
        ans += dfs(loc+ADJ_U, color, llp) + dfs(loc+ADJ_D, color, llp) + dfs(loc+ADJ_L, color, llp) + dfs(loc+ADJ_R, color, llp);
        return ans;
    }
};

bool check_row_col(const vector<int>& vec, int i, int M, int N, vector<Point>& temp) {
    temp.clear();
    for(; i < vec.size(); i += 2) {
        int row = vec[i], col = vec[i+1];
        if(row < 0 || row >= M || col < 0 || col >= N) {
            printf("(%d,%d) : row index range is [0,%d], column index range is [0,%d]\n", row, col, M-1, N-1);
            return false;
        }
        temp.emplace_back(row, col);
    }
    return true;
}

void remove_duplicate(vector<Point>& src, Point& lu, Point& rd, bool shift) {// 去重
    if(src.empty()) return;
    for(Point& p : src) {
        lu.row = min(lu.row, p.row);
        lu.col = min(lu.col, p.col);
        rd.row = max(rd.row, p.row);
        rd.col = max(rd.col, p.col);
    }
    if(shift) {
        rd -= lu;
        for(Point& p : src) p -= lu;
        lu.row = 0, lu.col = 0;
    }
    vector<vector<bool>> visit(rd.row+1, vector<bool>(rd.col+1, 0));
    for(Point& p : src) visit[p.row][p.col] = 1;
    src.clear();
    for(int i = 0; i <= rd.row; i++) {
        for(int j = 0; j <= rd.col; j++) if(visit[i][j]) src.emplace_back(i, j);
    }
}

bool parse_file(const char *path, Config& config) {
    ifstream file(path, ios::in);
    bool succ = true;
    if(!file) {
        printf("%s doesn't exist\n", path);
        succ = false;
    }
    string s;
    if(succ) {
        config.M = config.N = 0;
        config.holes.clear();
        config.pieces.clear();
        config.copies.clear();
    }
    while(succ && getline(file, s)) {
        if(s.empty() || s[0] == '#') continue;
        if(s[0] == 'D') {
            vector<int> vec = split(s, 1);
            if(vec.size() != 2 || vec[0] <= 0 || vec[1] <= 0) {
                printf("board dimension must be two positive integer, e.g. D:13 13\n");
                succ = false;break;
            }
            config.M = vec[0];
            config.N = vec[1];
        }
        else if((s[0] == 'P' || s[0] == 'H') && config.M == 0) {
            printf("board dimension must be set first\n");
            succ = false;break;
        }
        else if(s.size() >= 2 && s[0] == '~' && s[1] == 'D') break;
        else if(s[0] == 'P') {
            vector<int> p = split(s, 1);
            if(p.size() < 3 || !(p.size()&1) || p[0] < 1) {
                printf("piece format : first is how many copies, follow by row and column index pairs, e.g. P:1, 0 0, 1 0\n");
                succ = false;break;
            }
            vector<Point> temp;
            succ = check_row_col(p, 1, config.M, config.N, temp);
            if(!succ) break;
            config.pieces.emplace_back(move(temp));
            config.copies.push_back(p[0]);
        }
        else if(s[0] == 'H') {
            vector<int> h = split(s, 1);
            if(h.empty() || (h.size()&1)) {
                printf("holes format : row and column index pairs, e.g. H:0 0, 1 0\n");
                succ = false;break;
            }
            vector<Point> temp;
            succ = check_row_col(h, 0, config.M, config.N, temp);
            if(!succ) break;
            config.holes.insert(config.holes.end(), temp.begin(), temp.end());
        }
    }
    if(succ) {
        Point lu(INT_MAX, INT_MAX), rd(INT_MIN, INT_MIN);
        remove_duplicate(config.holes, lu, rd, false);
        for(vector<Point>& vec : config.pieces) {
            Point lu(INT_MAX, INT_MAX), rd(INT_MIN, INT_MIN);
            remove_duplicate(vec, lu, rd, true);
            config.corners.push_back(rd);
        }
        int cnt = 0, total = config.M * config.N - config.holes.size();
        for(int i = 0; i < config.copies.size(); i++) cnt += config.copies[i] * config.pieces[i].size();
        succ = cnt == total;
        if(!succ) printf("total area covered by the pieces (%d) and the area of the solution (%d) must be equal\n", cnt, total);
    }
    file.close();
    return succ && config.pieces.size() > 0;
}

void print_help(const char *exe) {
    printf("%s [LLP [CheckIsoFreq [info]]] file\n", exe);
    printf("LLP: 0 or 1. default 1. if 1, the solver always solve the least liberties point first\n");
    printf("CheckIsoFreq: nonnegative integer. default 0. if positive, it defines how often the solver should check for isolated areas\n");
    printf("info: 0 or 1. default 0. if 1, the process will be printed out\n");
    printf("file: puzzle config file path. format: D(Dimension), P(Piece), H(Hole), e.g.\n");
    printf("D:2 3\n");
    printf("P:1, 0 0, 1 0\n");
    printf("P:2, 0 0\n");
    printf("H:0 0, 1 1\n");
    printf("~D\n");
    exit(-1);
}

int main(int argc, char const *argv[]) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    system("cls");
#endif
    if(argc < 2 || argc > 5) print_help(argv[0]);
    try {
        bool llp = true, info = false;
        int freq = 0;
        if(argc > 2) llp = atoi(argv[1]);
        if(argc > 3) freq = atoi(argv[2]);
        if(argc > 4) info = atoi(argv[3]);
        Config config;
        string path(argv[argc-1]);
        if(parse_file(path.c_str(), config)) {
            path += ".ans";
            ofstream out_file(path.c_str(), ios::out);
            if(!out_file) {
                printf("%s doesn't exist\n", path.c_str());
                out_file.close();
                exit(-1);
            }
            Solver solver(config, out_file, freq, llp, info);
            solver.solve();
            out_file.close();
        }
        else printf("please check %s\n", argv[argc-1]);
    }
    catch(exception& e) {
        cout << e.what() << endl;
    }
    return 0;
}
