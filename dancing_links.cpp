#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <tuple>
#include <fstream>
using namespace std;

// 精确覆盖问题
// algorithm X + dancing links
struct DLX {
    bool opt;// 选择列的时候是否优先选择分支少的列
    int opt_col;
    int K = 0;// 类别数 0:任意两行都属于不同类别，也就是不考虑类别，但必须覆盖全部列  >0:每一类别必须选择一行，一共K行
    int cnt;// 节点个数
    vector<int> L = {0}, R = {0}, U, D, Row, Col, Size, temp;
    vector<vector<int>> ans;
    DLX(vector<vector<bool>>& mat, int k, bool opt=true):opt(opt) {
        if(mat.empty() || mat[0].empty() || k < 0 || (k && k > mat.size())) return;
        init(mat[0].size(), k);
        add(mat);
    }
    DLX(vector<vector<int>>& index, int col, int k, bool opt=true):opt(opt) {// index:每一行1的列索引
        if(index.empty() || col <= 0 || k < 0 || (k && k > index.size())) return;
        init(col, k);
        int i = 0;// 矩阵行索引
        for(vector<int>& idx : index) {
            for(int& c : idx) add(i, c);
            i++;
        }
    }
    void init(int col, int k) {// col:矩阵列数
        K = k;
        opt_col = col;
        if(opt && K > 0) opt_col = K;// K>0时前面K列表示类别，必须全部覆盖，只能在前K列里面选择分支少的列
        cnt = col + 1;
        L = vector<int>(cnt, 0);
        R = vector<int>(cnt, 0);
        U = vector<int>(cnt, 0);
        D = vector<int>(cnt, 0);
        Row = vector<int>(cnt, -1);
        Col = vector<int>(cnt, 0);
        Size = vector<int>(cnt, 0);
        for(int i = 0; i <= col; i++) {// 总头结点，每一列的头结点
            L[i] = i - 1;
            R[i] = i + 1;
            U[i] = i;
            D[i] = i;
            Col[i] = i;
        }
        L[0] = col;
        R[col] = 0;
    }
    void add(int row, int col) {// 按顺序添加矩阵中的1
        int u = U[++col];
        D[u] = cnt;
        U.push_back(u);
        U[col] = cnt;
        D.push_back(col);
        R.push_back(cnt);// 暂时指向自己
        L.push_back(cnt);// 暂时指向自己
        if(Row[cnt-1] == row) {// 和前一次插入的1在同一行
            int r = R[cnt-1];
            R[cnt] = r;
            L[r] = cnt;
            R[cnt-1] = cnt;
            L[cnt] = cnt - 1;
        }
        Size[col]++;
        Col.push_back(col);
        Row.push_back(row);
        cnt++;
    }
    void add(vector<vector<bool>>& mat) {
        int m = mat.size(), n = mat[0].size();
        for(int i = 0; i < m; i++) {
            for(int j = 0; j < n; j++) {
                if(mat[i][j]) add(i, j);
            }
        }
    }
    void backtrace(int k) {
        if(k == 0) printf("%d\n", k);
        int c = R[0];
        if((!K && !c) || (K && k == K)) {// 覆盖全部列或者恰好选择K行，找到一个解
            ans.push_back(temp);
            return;
        }
        if(opt) {// 优先选择分支少的列
            for(int i = c, s = INT_MAX; i != 0 && i <= opt_col; i = R[i]) {
                if(Size[i] < s) {
                    s = Size[i];
                    c = i;
                }
                if(s == 0) break;// 0是最小值
            }
        }
        if(Size[c] == 0) return;// 后面的代码无效
        cover(c);
        for(int i = D[c]; i != c; i = D[i]) {// 遍历列c中1所在的行
            temp.push_back(Row[i]);// 选择i所在的行
            for(int j = R[i]; j != i; j = R[j]) cover(Col[j]);// 该行同时覆盖了多列
            backtrace(k+1);
            for(int j = L[i]; j != i; j = L[j]) uncover(Col[j]);// 撤销覆盖
            temp.pop_back();// 撤销选择
        }
        uncover(c);
    }
    void cover(int col) {// 覆盖列col
        L[R[col]] = L[col];
        R[L[col]] = R[col];
        for(int i = D[col]; i != col; i = D[i]) {
            for(int j = R[i]; j != i; j = R[j]) {
                U[D[j]] = U[j];
                D[U[j]] = D[j];
                Size[Col[j]]--;
            }
        }
    }
    void uncover(int col) {// 取消覆盖列col
        for(int i = U[col]; i != col; i = U[i]) {
            for(int j = L[i]; j != i; j = L[j]) {
                U[D[j]] = j;
                D[U[j]] = j;
                Size[Col[j]]++;
            }
        }
        L[R[col]] = col;
        R[L[col]] = col;
    }
};

vector<int> split(string& s, char delimiter=' ') {
    vector<int> ans;
    size_t i = s.find_first_not_of(delimiter), j;
    while(i != string::npos) {
        j = s.find_first_of(delimiter, i+1);
        ans.push_back(stoi(s.substr(i, j-i)));
        i = s.find_first_not_of(delimiter, j);
    }
    return ans;
}
vector<int> split(string& s, string delimiter=": ") {
    vector<int> ans;
    size_t i = s.find_first_not_of(delimiter), j;
    while(i != string::npos) {
        j = s.find_first_of(delimiter, i+1);
        ans.push_back(stoi(s.substr(i, j-i)));
        i = s.find_first_not_of(delimiter, j);
    }
    return ans;
}

void print_error(ifstream& f, char *file, int row) {
    if(row >= 0) printf("please check the data in %s (row index:%d)!\n", file, row);
    else printf("%s doesn't exist!\n", file);
    f.close();
    exit(-1);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s file_dir\n", argv[0]);
        printf("file_dir : input file path\n");
        exit(-1);
    }
    char *file_dir = argv[1];
    ifstream file(file_dir, ios::in);
    if(!file) print_error(file, file_dir, -1);
    string str;
    int row = 0, ROW = 0, COL = 0, K = 0;
    getline(file, str);
    vector<int> vec = split(str, ' ');
    if(vec.size() != 2 && vec.size() != 3) print_error(file, file_dir, row);
    ROW = vec[0];COL = vec[1];
    if(vec.size() == 3) K = vec[2];
    if(ROW < 1 || COL < 1 || K < 0) print_error(file, file_dir, row);
    vector<vector<int>> mat;
    while(row < ROW && getline(file, str)) {
        mat.emplace_back(split(str, ": "));
        row++;
        vector<int>& idx = mat.back();
        int cnt = idx.size();
        if((!K && !cnt) || (K && (cnt < 2 || idx[0] < 0 || idx[0] >= K))) print_error(file, file_dir, row);
        for(int i = K?1:0; i < cnt; i++) {
            if(idx[i] < 0 || idx[i] >= COL) print_error(file, file_dir, row);
            else if(K) idx[i] += K;// 加上偏移量
        }
    }
    if(row != ROW) print_error(file, file_dir, ROW);// 行数不够
    file.close();
    string ans_dir(file_dir);ans_dir += ".ans";
    ofstream ans_file(ans_dir, ios::out);
    if(!ans_file) {
        printf("Failed to create %s!\n", ans_dir.c_str());
        ans_file.close();
        exit(-1);
    }
    DLX dlx(mat, COL+K, K, true);
    dlx.backtrace(0);
    ans_file << dlx.ans.size() << endl;
    for(vector<int>& a : dlx.ans) {
        ans_file << a[0];
        for(int i = 1; i < a.size(); i++) ans_file << ' ' << a[i];
        ans_file << endl;
    }
    for(vector<int>& a : dlx.ans) {
        ans_file << endl;
        for(int& i : a) {
            ans_file << mat[i][0];
            if(K) ans_file << ":" << mat[i][1];
            for(int j = K?2:1; j < mat[i].size(); j++) ans_file << ' ' << mat[i][j]-K;
            ans_file << endl;
        }
    }
    ans_file.close();
    return 0;
}
