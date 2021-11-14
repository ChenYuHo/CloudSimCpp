import sys
#sys.path.insert(0,'/usr/local/lib/python3.7/site-packages')

import pandas as pd
import numpy as np
import re
import math
from os import path
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import matplotlib.dates as dates
import matplotlib
from pandas.plotting import register_matplotlib_converters
register_matplotlib_converters()
from collections import defaultdict

# ----- Figure Style ----- #
# print(plt.style.available) # Uncomment to print available styles
plt.style.use('seaborn-talk')
# plt.style.use(['science','ieee','no-latex'])


# # ----- Font Size ----- #
# FONT_SIZE = 30
# matplotlib.rc('xtick', labelsize=FONT_SIZE) 
# matplotlib.rc('ytick', labelsize=FONT_SIZE)
# matplotlib.rcParams.update({'font.size': FONT_SIZE})

# # ----- Figure Size ----- #
# FIGURE_SIZE     = (10, 10)
# FIGURE_DPI      = 300
# plt.rcParams["figure.figsize"] = FIGURE_SIZE

def parse_fp(line):
    parts = line.split()
    # [forward] iter 2 jid 4 mid 6 tid 8 size 10 start 12 duration 14 end 16
    return (int(parts[8]), int(parts[12])/1e9, int(parts[14])/1e9, int(parts[16])/1e9)

def plot_timeline(fname, rank=0, from_iter=0, iter_num=1, jid=0, y0=0, height=1, merge_tensor=False, merge_iter=False, plot_this_fp=True, plot_next_fp=False, ax=None, xlim=None, alpha=1):
    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:brown', 'tab:pink', 'tab:gray', 'tab:olive', 'tab:cyan', 'blue', 'orange', 'green', 'red', 'purple', 'brown', 'pink', 'gray', 'olive', 'cyan']
    
    with open(fname) as f:
        lines = [l for l in f if f'rank {rank}' in l]
        
    if not ax:
        fig, ax = plt.subplots()
    
    for n_iter in range(from_iter, from_iter+iter_num):
        bp = [parse_fp(l) for l in lines if f'[backward] iter {n_iter}' in l and f'jid {jid}' in l]
        bp_start = min(start for _,start,_,_ in bp)
        bp_end = max(end for _,_,_,end in bp)
        if not merge_iter:
            if merge_tensor:
                ax.broken_barh([(bp_start, bp_end-bp_start)], (y0+height, height*.8), facecolors=[colors[n_iter%len(colors)]], alpha=alpha, hatch = '...')
            else:
                ax.broken_barh([(start,duration) for _,start,duration,_ in bp], (y0+height, height*.8), facecolors=[colors[tid] for tid,_,_,_ in bp], alpha=alpha, hatch = '...')

        fp = [parse_fp(l) for l in lines if f'[forward] iter {n_iter}' in l and f'jid {jid}' in l]
        fp_start = min(start for _,start,_,_ in fp)
        fp_end = max(end for _,_,_,end in fp)
        if not merge_iter:
            if plot_this_fp:

                if merge_tensor:
                    ax.broken_barh([(fp_start, fp_end-fp_start)], (y0+height, height*.8), facecolors=[colors[(n_iter+1)%len(colors)]], alpha=alpha)
                else:
                    ax.broken_barh([(start,duration) for _,start,duration,_ in fp], (y0+height, height*.8), facecolors=[colors[tid] for tid,_,_,_ in fp], alpha=alpha)

            if plot_next_fp:
                fp = [parse_fp(l) for l in lines if f'[forward] iter {n_iter+1}' in l and f'jid {jid}' in l]
                fp_start = min(start for _,start,_,_ in fp)
                fp_end = max(end for _,_,_,end in fp)
                if merge_tensor:
                    ax.broken_barh([(fp_start, fp_end-fp_start)], (y0+height, height*.8), facecolors=[colors[(n_iter+1)%len(colors)]], alpha=alpha)
                else:
                    ax.broken_barh([(start,duration) for _,start,duration,_ in fp], (y0+height, height*.8), facecolors=[colors[tid] for tid,_,_,_ in fp], alpha=alpha)

        allreduce = [parse_fp(l) for l in lines if f'[allreduce] iter {n_iter}' in l and f'jid {jid}' in l]
        if allreduce:
            allreduce_start = min(start for _,start,_,_ in allreduce)
            allreduce_end = max(end for _,_,_,end in allreduce)

            allreduce_time = defaultdict(float)
            for tid,_,duration,_ in allreduce:
                allreduce_time[tid] += duration

            if not merge_iter:
                if merge_tensor:
                    ax.broken_barh([(allreduce_start, allreduce_end-allreduce_start)], (y0, height*.8), facecolors=[colors[(n_iter+2)%len(colors)]], alpha=alpha)
                else:
                    ax.broken_barh([(start,duration) for _,start,duration,_ in allreduce], (y0, height*.8), facecolors=[colors[tid] for tid,_,_,_ in allreduce], alpha=alpha)
            # print(allreduce)

            if merge_iter:
                ax.broken_barh([(fp_start, allreduce_end-fp_start)], (y0, height*.8), facecolors=[colors[(n_iter)%len(colors)]], alpha=alpha)
    return plt, ax

# input = sys.argv[0] if len(sys.argv)>1 else sys.stdin
plt, ax = plot_timeline(sys.argv[1], plot_next_fp=True, from_iter=0, iter_num=2)
plt, ax = plot_timeline(sys.argv[1], from_iter=0, iter_num=2, jid=1, y0=2, ax=ax, rank=1)
plt.show()
