'''
Given a list of algos, prints them in standard Rubik's cube notation.

Reads from stdin, writes to stdout.
Ignores lines with --dashes--.
'''

import sys
from itertools import pairwise

def pretty(alg):
    if len(alg) == 1:
        if alg.islower():
            return alg.upper() + "' "
        # else
        return alg + " "

    if len(alg) == 0:
        return " "

    if alg[0] == alg[1]:
        return alg[0].upper() + "2 " + pretty(alg[2:])

    return pretty(alg[0]) + pretty(alg[1:])

for line in sys.stdin:
    if line[0] == '-':
        continue

    alg = line[:-1] # line[-1] == '\n'
    print(pretty(alg)[:-1]) # pretty outputs trailing space bc im lazy
