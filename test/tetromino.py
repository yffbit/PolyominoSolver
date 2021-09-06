from matplotlib import pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib import cm
from matplotlib.collections import PatchCollection
# 拼板尺寸
board = (3, 7)
# 拼图部件，四连方(tetromino)，一共5种
# 部件间可以接触
# 0:  ####
# 1:  ##
#     ##
# 2:  ###   #   #    #
#      #   ###  ##  ##
#               #    #
# 3:  ##    ##   #  #
#      ##  ##   ##  ##
#               #    #
# 4:  #      #  ###  ###  ##  ##  #    #
#     ###  ###  #      #  #    #  #    #
#                         #    #  ##  ##
pieces = [
    [(1,4),[0,1,2,3]],
    [(2,2),[0,1,2,3]],
    [(2,3),[0,1,2,4],
     (2,3),[1,3,4,5],
     (3,2),[0,2,3,4],
     (3,2),[1,2,3,5]
    ],
    [(2,3),[0,1,4,5],
     (2,3),[1,2,3,4],
     (3,2),[1,2,3,4],
     (3,2),[0,2,3,5]
    ],
    [(2,3),[0,3,4,5],
     (2,3),[2,3,4,5],
     (2,3),[0,1,2,3],
     (2,3),[0,1,2,5],
     (3,2),[0,1,2,4],
     (3,2),[0,1,3,5],
     (3,2),[0,2,4,5],
     (3,2),[1,3,4,5]
    ]
]
# 输出时每个部件代表的字符
pieces_str = ['0','1','2','3','4']

def cover_index(shape, board):
    """把图案的最小外切矩形放在拼板的左上角时,占据的索引
    shape : 图案最小外切矩形的尺寸
    board : 拼板的尺寸
    """
    col = board[1]
    index = []
    for i in range(shape[0]):
        for j in range(shape[1]):
            index.append(i*col+j)
    return index

def possible_index_list(shape, inner_idx, board):
    rec_index = cover_index(shape, board)
    index = [rec_index[i] for i in inner_idx]
    index_list = []
    col = board[1]
    for i in range(board[0]-shape[0]+1):
        for j in range(col-shape[1]+1):
            # (i,j)相对于(0,0)的偏移量
            offset = i * col + j
            index_list.append([v+offset for v in index])
    return index_list

pieces_index_list = []
pieces_index_set = []
for piece in pieces:
    index_list = []
    for j in range(0, len(piece), 2):
        shape = piece[j]
        inner_idx = piece[j+1]
        temp = possible_index_list(shape, inner_idx, board)
        index_list.extend(temp)
    pieces_index_list.append(index_list)
    index_set = [set(v_list) for v_list in index_list]
    pieces_index_set.append(index_set)
print([len(v_list) for v_list in pieces_index_list])
ROW = sum([len(p) for p in pieces_index_list])

def save_file_dlx(file_name):
    with open(file_name, 'w') as f:
        f.write('%d %d %d\n' % (ROW, board[0]*board[1], len(pieces_index_list)))# 矩阵行列，部件种数
        for i,piece_index in enumerate(pieces_index_list):
            for index in piece_index:
                f.write(str(i)+':'+' '.join([str(i) for i in index]))
                f.write('\n')

# save_file_dlx('pentomino_dlx.txt')

def save_html(file_name, board, pieces_index_list):
    """Gerard's Polyomino Solver"""
    row, col = board
    n = len(pieces_index_list)
    cnt = sum([len(index[0]) for index in pieces_index_list])
    empty = row * col - cnt
    if empty < 0:
        return
    elif empty > 0:
        n = n + 1
    with open(file_name, 'w') as f:
        f.write('<HTML>\n')
        f.write("<TITLE>Gerard's Polyomino Solver</TITLE>\n")
        f.write('<BODY>\n')
        f.write('<APPLET CODE="Polyomino.class" archive="solver.jar" WIDTH=400 HEIGHT=300 ALIGN=bottom>\n')
        f.write('<PARAM NAME=Version VALUE=2>\n')
        f.write('<PARAM NAME=PuzzleWidth VALUE=%d>\n' % board[1])
        f.write('<PARAM NAME=PuzzleHeight VALUE=%d>\n' % board[0])
        f.write('<PARAM NAME=NumPieces VALUE=%d>\n' % n)
        for i,index in enumerate(pieces_index_list):
            f.write('<PARAM NAME=Piece%d VALUE="true,1,%d' % (i+1, len(index[0])))
            for i in index[0]:
                f.write(',%d,%d' % (i//col, i%col))
            f.write('">\n')
        if empty > 0:
            f.write('<PARAM NAME=Piece%d VALUE="false,%d,1,0,0">\n' % (n, empty))
        f.write('<PARAM NAME=Holes VALUE="">\n')
        f.write('<PARAM NAME=CheckIsoFreq VALUE=0>\n')
        f.write('<PARAM NAME=AFPInSmallest VALUE=false>\n')
        f.write('</APPLET>\n')
        f.write('</BODY>\n')
        f.write('</HTML>\n')

save_html('../PolyominoSolver/run_pentomino.html', board, pieces_index_list)

# 回溯求解
ans = []
curr_index = []
curr_set = set()
num_pieces = len(pieces)
def backtrace(i, curr_set, curr_index, ans, num_pieces, pieces_index_set):
    """DFS回溯求解"""
    if(i == num_pieces):
        ans.append(curr_index.copy())
        return
    for j in range(len(pieces_index_set[i])):
        temp_set = pieces_index_set[i][j]
        # 是否和已经放进拼板中的部件冲突
        if len(temp_set & curr_set) != 0:
            continue
        curr_index.append(j)
        curr_set = curr_set | temp_set
        backtrace(i+1, curr_set, curr_index, ans, num_pieces, pieces_index_set)
        curr_set = curr_set - temp_set
        curr_index.pop()
backtrace(0, curr_set, curr_index, ans, num_pieces, pieces_index_set)

def transform(src, row, col, type):
    res = [src[i*col:(i+1)*col] for i in range(row)]
    if type % 2:# 水平镜像
        for i in range(row):
            res[i].reverse()
    type = type // 2
    if type % 2:# 垂直镜像
        res.reverse()
    type = type // 2
    if type % 2 and row == col:# 转置
        res = [[res[i][j] for i in range(row)] for j in range(col)]
    res1 = []
    for i in range(row):
        res1.extend(res[i])
    return res1

def unique_ans(ans, board, pieces_index_list, pieces_str):
    """结果去重,水平镜像,垂直镜像,转置等变换得到的解只统计一次"""
    row, col = board
    K = 8 if row == col else 4
    str_set = set()
    res = []
    for a in ans:
        curr_board = [' '] * (row*col)
        for i, j in enumerate(a):
            for k in pieces_index_list[i][j]:
                curr_board[k] = pieces_str[i]
        curr_str = ''.join(curr_board)
        if curr_str in str_set:
            continue
        str_set.add(curr_str)
        res.append(a.copy())
        for k in range(1, K, 1):# 当前解的所有变换
            temp_board = transform(curr_board, row, col, k)
            str_set.add(''.join(temp_board))
    return res
u_ans = unique_ans(ans, board, pieces_index_list, pieces_str)
print(len(ans))
print(len(u_ans))

def show_fig(index, shape):
    mycolors = cm.get_cmap('tab20').colors
    row, col = shape
    ax = plt.subplot(111)
    plt.xlim([0, col])
    plt.ylim([0, row])
    rect = []
    for i,idx in enumerate(index):
        for j in idx:
            y, x = j // col, j % col
            y = row - 1 - y
            t = Rectangle((x, y), 1, 1, color=mycolors[i])
            rect.append(t)
    ax.add_collection(PatchCollection(rect, match_original=True))
    ax.set_aspect('equal')
    ax.set_xticks(range(col+1))
    ax.set_yticks(range(row+1))
    # plt.axis('off')
    plt.grid()
    plt.show()

def print_result(ans, board, pieces_index_list, pieces_str, show=False):
    """输出所有解"""
    row, col = board
    for a in ans:
        curr_board = [' '] * (row*col)
        index = []
        for i, j in enumerate(a):
            index.append(pieces_index_list[i][j])
            for k in pieces_index_list[i][j]:
                curr_board[k] = pieces_str[i]
        for i in range(row):
            print(''.join(curr_board[i*col:(i+1)*col]))
        print()
        if not show:
            return
        show_fig(index, board)

print_result(ans, board, pieces_index_list, pieces_str)
print_result(u_ans, board, pieces_index_list, pieces_str, True)
