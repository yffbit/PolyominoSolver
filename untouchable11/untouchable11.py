from matplotlib import pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib import cm
from matplotlib.collections import PatchCollection
import numpy as np
import sys
import os
os.environ['KMP_DUPLICATE_LIB_OK'] = 'TRUE'

# 拼板尺寸
board = (12, 12)
# 拼图部件，六连方(hexomino)，一共35种
# 取其中的11种，都是立方体的展开图
# 任意两个部件之间不能接触
# 0:   #      #    #    #
#     ####  ####   #   ###
#      #      #   ###   #
#                  #    #
# 1:  #        #   #   ###
#     ####  ####   #    #
#     #        #   #    #
#                 ###   #
# 2:  #        #   ##  ##
#     ####  ####   #    #
#        #  #      #    #
#                 ##    ##
# 3:  ###      ###   #  #
#       ###  ###     #  #
#                    #  #
#                   #    #
#                   #    #
#                   #    #
# 4:   #      #    #    #
#     ####  ####   ##  ##
#       #    #    ##    ##
#                  #    #
# 5:  ##      ##    #  #
#      ##    ##    ##  ##
#       ##  ##    ##    ##
#                 #      #
# 6:  ##      ##   #      #    #    #   #      #
#      ###  ###    ###  ###    #    #   ###  ###
#      #      #   ##      ##  ###  ###   #    #
#                             #      #   #    #
# 7:  ##      ##    #    #     #    #   #      #
#      ###  ###    ###  ###    ##  ##   ##    ##
#       #    #    ##      ##  ##    ##   ##  ##
#                             #      #   #    #
# 8:  ##      ##     #  #      ##  ##   #      #
#      ###  ###    ###  ###    #    #   ##    ##
#        #  #     ##      ##  ##    ##   #    #
#                             #      #   ##  ##
# 9:  #        #   #      #    #    #   ##    ##
#     ####  ####  ####  ####   #    #    ##  ##
#      #      #   #        #   ##  ##    #    #
#                             ##    ##   #    #
# 10: #        #    #    #     #    #   ##    ##
#     ####  ####  ####  ####   ##  ##    #    #
#       #    #    #        #   #    #    ##  ##
#                             ##    ##   #    #
# 只给出每个部件的原型
pieces = [
    [(3,4),[1,4,5,6,7,9]],
    [(3,4),[0,4,5,6,7,8]],
    [(3,4),[0,4,5,6,7,11]],
    [(2,5),[0,1,2,7,8,9]],
    [(3,4),[1,4,5,6,7,10]],
    [(3,4),[0,1,5,6,10,11]],
    [(3,4),[0,1,5,6,7,9]],
    [(3,4),[0,1,5,6,7,10]],
    [(3,4),[0,1,5,6,7,11]],
    [(3,4),[0,4,5,6,7,9]],
    [(3,4),[0,4,5,6,7,10]]
]
# 输出时每个部件代表的字符
pieces_str = [chr(i+97) for i in range(11)]
plt_str = ['+','P','x','s','D','o','8','*','^','2','X']

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

def possible_index_list(shape, inner_idx, board, step):
    """如果是扩充前,step=1,如果是扩充后(原本的一格变成了2x2),step=2"""
    rec_index = cover_index(shape, board)
    index = [rec_index[i] for i in inner_idx]
    index_list = []
    col = board[1]
    for i in range(0, board[0]-shape[0]+1, step):
        for j in range(0, col-shape[1]+1, step):
            # (i,j)相对于(0,0)的偏移量
            offset = i * col + j
            index_list.append([v+offset for v in index])
    return index_list

def change_size(row, col):
    return 2*row+2, 2*col+2
def change_form(shape, inner_idx):
    """
    原问题两个部件之间不能接触,为了能够接触,在每个部件的周围增加半格长度,扩充后的部件就可以接触了
    为了使坐标都为整数,将一个格子看成2x2,也就是变成4个小格子,需要重新计算坐标
           ******
    ## --> *####*
           *####*
           ******
    """
    row, col = shape
    new_idx = set()
    for idx in inner_idx:
        r, c = idx // col, idx % col#扩充前的坐标
        # 部件的每个格子周围都扩充半格长度,扩充后占4x4,也就是16格
        # 部件中的相邻格子扩充后坐标有重复,通过集合去重
        r, c = 2 * r, 2 * c
        for i in range(r, r+4):
            for j in range(c, c+4):
                new_idx.add((i,j))
    row, col = change_size(row, col)
    new_idx = sorted([i*col+j for i,j in new_idx])
    return new_idx, (row, col)

def change_size1(row, col):
    return row+1, col+1
def change_form1(shape, inner_idx):
    """
    第二种转换方式
    将每个方块的四个顶点作为目标，对齐拼板上每个方块的顶点，将问题转化为可以相邻的传统拼图问题
    转化后将顶点作为主体，也就是把顶点看成方块，拼板的长宽都增加1
    ## --> ***
           ***
    """
    row, col = shape
    new_idx = set()
    for idx in inner_idx:
        r, c = idx // col, idx % col# 转换前坐标
        # 原来的方块的坐标作为左上角顶点的坐标，再加入其他三个角的坐标
        # 通过集合去重
        new_idx.add((r, c))
        new_idx.add((r, c+1))
        new_idx.add((r+1, c))
        new_idx.add((r+1, c+1))
    row, col = change_size1(row, col)
    new_idx = sorted([i*col+j for i,j in new_idx])
    return new_idx, (row, col)

def transform(src, row, col, type, ignore_rc=False):
    """
    src: 包含row*col个元素的list
    type:转换类型0~7,三位二进制bit0,bit1,bit2分别表示是否进行水平镜像,垂直镜像,转置三种变换
    ignore_rc: 转置时是否忽略行列不一致
    """
    res = [src[i*col:(i+1)*col] for i in range(row)]# 转成二维
    if type & 1:# 水平镜像
        for i in range(row):
            res[i].reverse()
    if type & 2:# 垂直镜像
        res.reverse()
    if type & 4 and (ignore_rc or row == col):# 转置
        res = [[res[i][j] for i in range(row)] for j in range(col)]
        row, col = col, row
    res1 = []# 转成一维后返回
    for i in range(row):
        res1.extend(res[i])
    return res1, (row, col)

def print_piece(src, row, col):
    for i in range(row):
        print(src[i*col:(i+1)*col])
    print()

mycolors = cm.get_cmap('tab20').colors
def show_fig(index, shape):
    """
    index: 二维list,每一行一组点的索引,这组点用同色的方块表示
    shape: (row,col),索引相关,用i=r*col+c表示二维索引(r,c)
    """
    row, col = shape
    fig = plt.figure()
    ax = fig.add_subplot(111)
    # ax = plt.subplot(111)
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

# debug = True
debug = False
def all_transform(shape, inner_idx, board):
    """每个部件所有可能的变换"""
    temp_set = set()
    res = [shape, inner_idx]
    row, col = shape
    src = [' '] * (row * col)
    for i in inner_idx:
        src[i] = '#'
    t = (shape, ''.join(src))
    temp_set.add(t)
    if debug:
        print_piece(t[1], *shape)
        show_fig([inner_idx], shape)
    for k in range(1, 8):
        temp, new_shape = transform(src, row, col, k, True)
        if new_shape[0] > board[0] or new_shape[1] > board[1]:# 超出边界
            continue
        t = (new_shape, ''.join(temp))
        if t in temp_set:# 重复
            continue
        if debug:
            print_piece(t[1], *new_shape)
            temp_idx = [i for i,val in enumerate(temp) if val == '#']
            show_fig([temp_idx], new_shape)
        res.append(new_shape)
        res.append([i for i in range(len(src)) if temp[i] == '#'])
        temp_set.add(t)
    return res

def all_pieces_index_list(pieces, board, form=1):
    pieces_index_list = []
    pieces_index_set = []
    if form == 1:
        new_board = change_size(*board)
        change_f = change_form
        step = 2
    else:
        new_board = change_size1(*board)
        change_f = change_form1
        step = 1
    for piece in pieces:
        index_list = []
        inner_idx, shape = change_f(piece[0], piece[1])
        piece = all_transform(shape, inner_idx, new_board)
        for i in range(0, len(piece), 2):
            temp = possible_index_list(piece[i], piece[i+1], new_board, step)
            index_list.extend(temp)
        pieces_index_list.append(index_list)
        index_set = [set(v_list) for v_list in index_list]
        pieces_index_set.append(index_set)
    return pieces_index_list, pieces_index_set

def print_help():
    print('python '+sys.argv[0]+' form')
    print('form : 1 or 2')
    exit(-1)
if len(sys.argv) < 2:
    print_help()
form = int(sys.argv[1])
if form != 1 and form != 2:
    print_help()
if form == 1:
    change_s = change_size
else:
    change_s = change_size1
pieces_index_list, pieces_index_set = all_pieces_index_list(pieces, board, form)
new_board = change_s(*board)
print([len(v_list) for v_list in pieces_index_list])
ROW = sum([len(p) for p in pieces_index_list])
print(ROW)

def save_file_dlx(file_name):
    """DLX"""
    with open(file_name, 'w') as f:
        f.write('%d %d %d\n' % (ROW, new_board[0]*new_board[1], len(pieces_index_list)))# 矩阵行列，部件种数
        for i,piece_index in enumerate(pieces_index_list):
            for index in piece_index:
                f.write(str(i)+':'+' '.join([str(i) for i in index]))
                f.write('\n')

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

def save_config_file(file_name, board, pieces_index_list):
    """Solver"""
    row, col = board
    n = len(pieces_index_list)
    cnt = sum([len(index[0]) for index in pieces_index_list])
    empty = row * col - cnt
    if empty < 0:
        return
    elif empty > 0:
        n = n + 1
    with open(file_name, 'w') as f:
        f.write('D:%d %d\n' % (board[0], board[1]))
        for i,index in enumerate(pieces_index_list):
            f.write('P:1')
            for i in index[0]:
                f.write(', %d %d' % (i//col, i%col))
            f.write('\n')
        if empty > 0:
            f.write('P:%d, 0 0\n' % empty)
        f.write('~D\n')

save_file_dlx('untouchable11_form%d_dlx.txt'%form)
save_html('../PolyominoSolver/run_untouchable11_form%d.html'%form, new_board, pieces_index_list)
save_config_file('untouchable11_form%d_test.txt'%form, new_board, pieces_index_list)


# 回溯求解
def backtrace(i, curr_set, curr_index, ans, num_pieces, pieces_index_set):
    """DFS回溯求解"""
    if(i == num_pieces):
        ans.append(curr_index.copy())
        print(curr_index)
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

ans = []
curr_index = []
# curr_set = set()
num_pieces = len(pieces)
# backtrace(0, curr_set, curr_index, ans, num_pieces, pieces_index_set)

def all_pieces_index_num(pieces_index_list):
    """拼板上每个索引对应二进制的一位,被覆盖表示二进制位为1,将每个图案在拼板中覆盖的索引转成数字"""
    pieces_index_num = []
    for piece_index in pieces_index_list:
        num = []
        for index in piece_index:
            t = 0
            for i in index:
                t = t + (1 << i)
            num.append(t)
        pieces_index_num.append(num)
    return pieces_index_num
def all_pieces_index_mat(pieces_index_list, shape):
    pieces_index_mat = []
    N = shape[0] * shape[1]
    for piece_index in pieces_index_list:
        mat = []
        for index in piece_index:
            t = np.zeros(N, dtype='bool')
            t[np.array(index)] = True
            mat.append(t.reshape(shape))
        pieces_index_mat.append(mat)
    return pieces_index_mat

def add_to_set(board_set, mat):
    board_set.add(mat.tobytes())
    for i in range(1, 8):# 水平镜像,垂直镜像,转置(拼板长宽相等)等变换进行组合
        temp = mat.copy()
        if i & 1:
            temp = np.fliplr(temp)
        if i & 2:
            temp = np.flipud(temp)
        if i & 4:
            temp = np.transpose(temp)
        board_set.add(temp.tobytes())

pieces_index_mat = all_pieces_index_mat(pieces_index_list, new_board)
print([len(m) for m in pieces_index_mat])
curr_mat = np.zeros(new_board, dtype='bool')
board_set = [set() for _ in range(num_pieces)]
pieces_index = []
# debug = True
def backtrace1(i, curr_mat):
    """DFS回溯求解"""
    if(i == num_pieces):
        ans.append(curr_index.copy())
        print(curr_index)
        return
    for j,mat in enumerate(pieces_index_mat[i]):
        if i == 0:
            print('%d:%d' % (i,j+1))
        if np.bitwise_and(mat, curr_mat).any():# 和已经放进拼板中的部件冲突
            continue
        t = np.bitwise_xor(curr_mat, mat)
        if t.tobytes() in board_set[i]:# 同构局面已经搜索过
            continue
        add_to_set(board_set[i], t)
        curr_index.append(j)
        curr_mat = t
        if debug:
            pieces_index.append(pieces_index_list[i][j])
            temp = np.zeros_like(curr_mat, dtype='str')
            temp.fill(' ')
            temp[curr_mat] = '#'
            print('~'*(temp.shape[1]+2))
            for s in temp:
                print('|'+''.join(s)+'|')
            print('~'*(temp.shape[1]+2))
            show_fig(pieces_index, new_board)
        backtrace1(i+1, curr_mat)
        curr_mat = np.bitwise_xor(curr_mat, mat)
        curr_index.pop()
        if debug:
            pieces_index.pop()
# backtrace1(0, curr_mat)

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
            temp_board, _, _ = transform(curr_board, row, col, k)
            str_set.add(''.join(temp_board))
    return res

u_ans = unique_ans(ans, new_board, pieces_index_list, pieces_str)
print(len(ans))
print(len(u_ans))

def print_result(ans, board, pieces_index_list, pieces_str, plt_str, show=False):
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
        for i in range(len(index)):
            x = [j%col for j in index[i]]
            y = [j//col for j in index[i]]
            plt.plot(x, y, plt_str[i])
        plt.gca().set_aspect('equal')
        plt.show()
print_result(ans, board, pieces_index_list, pieces_str, plt_str)
print_result(u_ans, board, pieces_index_list, pieces_str, plt_str, True)
