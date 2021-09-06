import sys
import os
import math
from matplotlib import pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib import cm
from matplotlib.collections import PatchCollection
os.environ['KMP_DUPLICATE_LIB_OK'] = 'TRUE'

def idx2rc(idx, col):
    return idx // col, idx % col
def rc2idx(i, j, col):
    return i * col + j

mycolors = cm.get_cmap('tab20').colors
def show_fig(index, shape):
    """
    index: 二维list,每一行一组点的索引,这组点用同色的方块表示
    shape: (row,col),索引相关,用i=r*col+c表示二维索引(r,c)
    """
    n = 0
    for idx in index:
        if idx:
            n += 1
    n = math.ceil(n / len(mycolors))
    temp_colors = mycolors * n
    if n > 1:
        print('colors will be used repeatly')
    row, col = shape
    fig = plt.figure()
    ax = fig.add_subplot(111)
    # ax = plt.subplot(111)
    plt.xlim([0, col])
    plt.ylim([0, row])
    rect = []
    for i,idx in enumerate(index):
        for j in idx:
            y, x = idx2rc(j, col)
            y = row - 1 - y
            t = Rectangle((x, y), 1, 1, color=temp_colors[i])
            rect.append(t)
    ax.add_collection(PatchCollection(rect, match_original=True))
    ax.set_aspect('equal')
    ax.set_xticks(range(col+1))
    ax.set_yticks(range(row+1))
    # plt.axis('off')
    plt.grid()
    plt.show()

def dfs(board, i, j, M, N, color, temp):
    if i < 0 or i >= M or j < 0 or j >= N or board[i][j] != color:
        return
    temp.append(rc2idx(i, j, N))
    board[i][j] = '.'
    dfs(board, i, j+1, M, N, color, temp)
    dfs(board, i+1, j, M, N, color, temp)
    dfs(board, i, j-1, M, N, color, temp)
    dfs(board, i-1, j, M, N, color, temp)

def get_index_form2(index, shape):
    """每个单位方格的四个顶点中,只保留左上角的顶点"""
    M, N = shape
    index1 = []
    for idx in index:
        temp = set(idx)
        temp = [i for i in idx if i+1 in temp and i+N in temp and i+N+1 in temp]
        temp = [rc2idx(i//N, i%N, N-1) for i in temp]
        index1.append(temp)
    return index1, (M-1, N-1)
def get_index_form1(index, shape):
    # 如果原坐标为(i,j),那么转换之后一共16个坐标
    # (2*i,2*j),(2*i,2*j+1),(2*i,2*j+2),(2*i,2*j+3)
    # (2*i+1,2*j),(2*i+1,2*j+1),(2*i+1,2*j+2),(2*i+1,2*j+3)
    # (2*i+2,2*j),(2*i+2,2*j+1),(2*i+2,2*j+2),(2*i+2,2*j+3)
    # (2*i+3,2*j),(2*i+3,2*j+1),(2*i+3,2*j+2),(2*i+3,2*j+3)
    # 除以2得到
    # (i,j),(i,j),(i,j+1),(i,j+1)
    # (i,j),(i,j),(i,j+1),(i,j+1)
    # (i+1,j),(i+1,j),(i+1,j+1),(i+1,j+1)
    # (i+1,j),(i+1,j),(i+1,j+1),(i+1,j+1)
    # 两个相邻的坐标(i,j),(i,j+1)转换之后会重复8个坐标
    # 对于变换后的每一个坐标(r,c),判断其他15个坐标存不存在,如果都存在,
    # 说明这16个坐标完全覆盖了一个格子,或者覆盖了两个相邻格子的部分
    # 将(r/2,c/2)加入集合
    M, N = shape
    M1, N1 = M//2-1, N//2-1
    index1 = []
    for idx in index:
        temp = set(idx)
        coord_set = set()
        for i in idx:
            flag = True
            for j in range(i, i+4*N, N):
                if j not in temp or j+1 not in temp or j+2 not in temp or j+3 not in temp:
                    flag = False
            if flag:
                coord_set.add(((i//N)//2, (i%N)//2))
        temp = sorted([rc2idx(r, c, N1) for r,c in coord_set])
        if temp:
            index1.append(temp)
    return index1, (M1, N1)

def show(board, form):
    if not board:
        return
    M = len(board)
    N = len(board[0])
    if N == 0:
        return
    for row in board:
        if len(row) != N:
            return
    index = []
    for i in range(M):
        for j in range(N):
            if board[i][j] != '.':
                temp = []
                dfs(board, i, j, M, N, board[i][j], temp)
                index.append(temp)
    show_fig(index, (M, N))
    if form == 1:
        index1, new_shape = get_index_form1(index, (M,N))
    elif form == 2:
        index1, new_shape = get_index_form2(index, (M,N))
    else:
        return
    show_fig(index1, new_shape)

def parse(file_path, form):
    with open(file_path, 'r') as f:
        board = []
        line = f.readline()
        while line:
            if line.startswith("sol") or line.startswith('Sol'):
                show(board, form)
                board = []
            else:
                line = line.strip()
                if line:
                    temp = line.split()
                    if len(temp) == 1:
                        board.append(list(line))
                    else:
                        board.append(temp)
            line = f.readline()
        if board:
            show(board, form)

if len(sys.argv) > 2 and os.path.isfile(sys.argv[2]):
    parse(sys.argv[2], int(sys.argv[1]))
else:
    print('python ' + sys.argv[0] + ' form file')
    print('form : 0:no conversion, 1 or 2:convert back to untouchable version by form 1 or 2')
    print('file : solution file path')
