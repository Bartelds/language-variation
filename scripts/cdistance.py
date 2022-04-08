import numpy as np
from scipy.spatial.distance import cdist
import ot

# pip install numpy scipy POT


def clusterify(data, labels):
    indices = sorted(set(labels))
    idx = [[] for _ in range(len(indices))]
    for x, l in zip(data, labels):
        idx[indices.index(l)].append(x)
    return idx


def cdistance(s1, s2, metric="euclidean"):
    # Step 1 - build pairwise optimal transport distance matrix between
    M = len(s1)
    N = len(s2)
    dprime = np.zeros((M, N))
    for i in range(M):
        for j in range(N):
            A = np.array(s1[i]); B = np.array(s2[j])
            m = len(A); n = len(B)

            # Compute (ground) distance matrix between points of each cluster
            domega = cdist(A, B, metric)

            # Assign equal weight to each point
            p = np.full((m), 1 / m)
            q = np.full((n), 1 / n)

            # Compute dprime entry
            dprime[i, j] = ot.emd2(p, q, domega)

    # Step 2 -
    # Assign weights to each point in new space, proportionate to number of
    # points in corresponding cluster
    p = np.zeros((M))
    for i in range(M):
        p[i] = len(s1[i])
    p = p / sum(p)

    q = np.zeros((N))
    for i in range(N):
        q[i] = len(s2[i])
    q = q / sum(q)

    # Compute Optimal Transportation Distance
    d_optimal = ot.emd2(p, q, dprime)

    # Compute Naive Transportation Distance
    d_naive = 0
    for i in range(M):
        for j in range(N):
            d_naive=d_naive+p[i]*q[j]*dprime[i,j]

    # Compute CDistance
    d=d_optimal/d_naive
    return d


if __name__ == "__main__":
    data1 = [(2, 10),
         (3, 9),
         (4, 10),
         (5, 3),
         (6, 2),
         (7, 3),
         (6, 1)]
     
    labels = [1, 1, 1, 2, 2, 2, 2]

    data2 = [(2, 10),
            (3, 9),
            (4, 10),
            (5, 3.8),
            (6, 2.8),
            (7, 3.8),
            (6, 1)
            ]

    data3 = [(2, 10),
            (3, 9),
            (4, 10),
            (5, 3.85),
            (6, 2.85),
            (7, 3.85),
            (6, 1)
            ]
    
    s1 = clusterify(data1, labels)
    s2 = clusterify(data2, labels)
    s3 = clusterify(data3, labels)

    print(f"CDistance2(S1,S2) = {cdistance(s1,s2):.4f}")
    print(f"CDistance2(S2,S3) = {cdistance(s2,s3):.4f}")
    print(f"CDistance2(S3,S1) = {cdistance(s3,s1):.4f}")

# Matlab:
# CDistance2(S1,S2) = 0.0861
# CDistance2(S2,S3) = 0.0059
# CDistance2(S3,S1) = 0.0914

# Python:
# CDistance2(S1,S2) = 0.0861
# CDistance2(S2,S3) = 0.0059
# CDistance2(S3,S1) = 0.0914
