import sys

def all_reorients(alg):
    '''
    Given `alg`, produces all algos that are equivalent up to some cube reorientation / flip.
    '''

    FLIP_Z = {
        "U": "u", "u": "U",
        "R": "r", "r": "R",
        "F": "b", "f": "B",
        "B": "f", "b": "F",
        "L": "l", "l": "L",
        "D": "d", "d": "D",
    }

    ROT_Y = {
        'U': 'U', 'u': 'u',
        'R': 'F', 'r': 'f',
        'F': 'L', 'f': 'l',
        'B': 'R', 'b': 'r',
        'L': 'B', 'l': 'b',
        'D': 'D', 'd': 'd',
    }

    ROT_X = {
        'U': 'B', 'u': 'b',
        'R': 'R', 'r': 'r',
        'F': 'U', 'f': 'u',
        'B': 'D', 'b': 'd',
        'L': 'L', 'l': 'l',
        'D': 'F', 'd': 'f',
    }

    # We compose the transformations to ensure we cover all 23 rotations and all 24 flips.
    # If this part looks opaque, it's because I found it by a semi-unsupervised search in Sagemath.
    ret = [alg]
    for _ in range(3):
        ret.append("".join(ROT_Y[move] for move in ret[-1]))
    for _ in range(12):
        ret.append("".join(ROT_X[move] for move in ret[-4]))
    for _ in range(16):
        ret.append("".join(FLIP_Z[move] for move in ret[-16]))
    for i in [0, 4, 8, 12]:
        for _ in range(4):
            ret.append("".join(ROT_Y[move] for move in ret[i - 28]))

    return set(ret) # Convert to set to remove duplicates

for line in sys.stdin:
    if line[0] == '-':
        continue

    alg = line[:-1] # line[-1] == '\n'
    for reorient in all_reorients(alg):
        print(reorient)
