import numpy as np
import scipy.cluster.hierarchy as hcluster
from scipy.cluster.vq import kmeans2
import scipy


def norm_along_axis(vecs):
    return np.sum(vecs**2, axis=-1)**(1./2)


def find_bases(qs, tolerance):
    offsets = np.empty((0, 3))
    for q in qs:
        bases = np.diagflat(q)
        bases = bases[np.any(bases, axis=0), :]
        offsets = np.vstack((offsets, bases))

    pools = {}
    indicies = []
    for q in offsets:
        added_to_pool = False
        for key, pool in pools.iteritems():
            result = np.absolute(q - round_nearest(q, pool))
            if np.all(result <= tolerance):
                added_to_pool = True
                indicies.append(key)
                break

        if not added_to_pool:
            key = len(pools.keys()) + 1
            pools[key] = q
            indicies.append(key)

    return indicies, len(pools), pools


def generate_grid(vecs, upper_bound):
    search_range = np.arange(-upper_bound, upper_bound)
    vs = np.array([v*i for v in vecs for i in search_range])
    vs = np.array([v+w for v in vs for w in vs if np.dot(v, w) == 0])
    return np.unique(vs, axis=0)


def round_to_nearest_reflection(qs, reflections, tolerance):
    kdtree = scipy.spatial.KDTree(reflections)

    for i, q in enumerate(qs):
        result = kdtree.query_ball_point(q, r=tolerance)
        if len(result) > 0:
            diff = q - reflections[result]
            idx = np.argsort(norm_along_axis(diff))
            qs[i] = reflections[result[idx[0]]]

    return qs


def index_q_vectors(qs, tolerance=.03):
    # choose the bases to represent each new axis
    idxs, ndim, pools = find_bases(qs, tolerance=tolerance)
    bases = np.row_stack(pools.itervalues())

    # generate all possible reflections from bases up to a given number
    MAX_REFLECTIONS = 10
    reflections = generate_grid(bases, MAX_REFLECTIONS)

    # round all reflections to the nearest reflection
    qs = round_to_nearest_reflection(qs, reflections, tolerance)

    ndim = 3+ndim
    basis = np.eye(ndim)
    basis[3:, :3] = bases
    basis = np.divide(1., basis, out=np.zeros_like(basis), where=basis != 0)

    qs_s = np.zeros((qs.shape[0], ndim))
    qs_s[:, :3] = qs

    # Find position of points in higher dimension space
    indicies = np.dot(qs_s, basis.T) - qs_s

    # We might have multiple translations along parallel directions in 3D
    # space. These will appear as fractional indicies in this matrix. We
    # remove them here by setting any no integer values to zero.
    indicies = remove_nonzero(indicies)
    indicies = indicies[:, 3:]

    return indicies


def cluster_qs(qs, k=None, threshold=1.5):
    if k is not None:
        centroid, clusters = kmeans2(qs, k)
    else:
        clusters = hcluster.fclusterdata(qs, threshold, criterion="distance")
    return clusters, len(set(clusters))


def average_clusters(qs, clusters):
    averaged_qs = []
    for cluster_index in set(clusters):
        averaged_qs.append(np.mean(qs[clusters == cluster_index], axis=0))
    return np.array(averaged_qs)


def get_hkls(peaks_workspace):
    return np.array([np.array([peak['h'], peak['k'], peak['l']])
                     for peak in peaks_workspace])


def remove_nonzero(M):
    M[np.absolute(np.mod(M, 1)) > 1e-14] = 0
    return M


def trunc_decimals(vec, n_decimals=2):
    decade = 10**n_decimals
    return np.trunc(vec*decade)/decade


def safe_divide(a, b):
    return np.divide(a, b, out=np.zeros_like(b), where=b != 0)


def round_nearest(x, a):
    return np.round(np.divide(x, a, out=np.zeros_like(a), where=a != 0)) * a
