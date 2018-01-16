import numpy as np
import scipy.cluster.hierarchy as hcluster
from scipy.cluster.vq import kmeans2
import scipy


def find_bases(qs, tolerance):
    """Find a set of bases from the list of qs

    This will estimate the number of additional dimensions needed to index the
    fractional peaks. One vector for each dimension will then be chosen and
    used as the basis for the new dimension.

    :param qs: list of q vectors to find bases for.
    :param tolerance: tolerance how close each vector needs to be to be classes
        as being the same
    :return: list of bases for each additional dimension required to index the
        fractional peaks.
    """
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
    """Generate a list of reflection positions

    This function will generate a list of reflection positions from a list of
    3D vectors up to some upper bound.

    :param vecs: the list of vectors to generate possible reflections from
    :param upper_bound: the upper bound for the number of reflections in each
        direction to generate
    :return: ndarray of reflections in every direction up to a given upper bound
    """
    search_range = np.arange(-upper_bound, upper_bound)
    vs = np.array([v*i for v in vecs for i in search_range])
    vs = np.array([v+w for v in vs for w in vs if np.dot(v, w) == 0])
    return np.unique(vs, axis=0)


def round_to_nearest_reflection(qs, reflections, tolerance):
    """Round the each raw q to the nearest predicted reflection

    :param qs: the list of raw noisy q vectors
    :param reflections: a list of predicted reflection locations.
    :param tolerance: the radius to search for neighbouring reflections in.
    :returns: ndarray of q values rounded to the nearest predicted reflection
    """
    kdtree = scipy.spatial.KDTree(reflections)

    for i, q in enumerate(qs):
        result = kdtree.query_ball_point(q, r=tolerance)
        if len(result) > 0:
            diff = q - reflections[result]
            idx = np.argsort(norm_along_axis(diff))
            qs[i] = reflections[result[idx[0]]]

    return qs


def index_q_vectors(qs, tolerance=.03):
    """Find the n-dimensional index of a collection of q vectors

    This function will automatically attempt infer the correct number of
    additional dimensions required to index the given list of q vectors.

    :param qs: the q vectors to find indicies for
    :param tolerance: the tolerance on whether two equivilent or multiple qs
        should be classifed as the same.
    :return: ndarray of indicies for each q vector
    """
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
    indicies = remove_noninteger(indicies)
    indicies = indicies[:, 3:]

    return indicies


def cluster_qs(qs, k=None, threshold=1.5):
    """Cluster q vectors into discrete groups.

    Classifies each of the q vectors into a number of clusters. The number of clusters used is decided by the parameters passed:
        * If the k parameter is supplied then the q vectors are grouped into k clusters using kmeans.
        * If the threshold parameter is supplied then the q vectors a split into groups based on cophenetic distance.

    :param qs: list of q vectors to cluster. Each element should be a numpy array of length three.
    :param k: number of clusters to use (optional).
    :param threshold: cophenetic distance cut off point for new clusters (optional)
    :returns: tuple (clusters, k)
        Where:
            list -- clusters is a list of cluster indicies which each q belongs to
            int -- k is the number of clusters used
    """
    if k is not None:
        _, clusters = kmeans2(qs, k, iter=30)
        if len(set(clusters)) != k:
            raise ValueError("Could not group the satellite reflections "
                             "into {} clusters. Please check that you have "
                             "at least {} satellites.".format(k,k))
    else:
        clusters = hcluster.fclusterdata(qs, threshold, criterion="distance")
    return clusters, len(set(clusters))


def average_clusters(qs, clusters):
    """Compute the centroid of each cluster of vectors

    :param qs: the q vectors to find the centroids of
    :param clusters: the cluster index that each q belongs to
    :return: an ndarray of cluster centroids
    """
    averaged_qs = []
    for cluster_index in set(clusters):
        averaged_qs.append(np.mean(qs[clusters == cluster_index], axis=0))
    return np.array(averaged_qs)


def get_hkls(peaks_workspace):
    """Get the H, K, and L columns from a PeaksWorkspace.

    :param peaks_workspace: the peaks workspace to extract HKL values from
    :return: 2D numpy array of HKL values.
    """
    return np.array([np.array([peak['h'], peak['k'], peak['l']])
                     for peak in peaks_workspace])


def remove_noninteger(M):
    """Remove any non integer values from a matrix

    :param M: the matrix to remove non integer values from
    :return: M with non integer elements set to zero
    """
    M[np.absolute(np.mod(M, 1)) > 1e-14] = 0
    return M


def trunc_decimals(vec, n_decimals=2):
    """Truncate the elements of a vector below n decimals places

    :param vec: the vector to truncate the elements of
    :param n_decimals: the number of decimal places to truncate from
    :return: the vector with truncated elements
    """
    decade = 10**n_decimals
    return np.trunc(vec*decade)/decade


def safe_divide(a, b):
    """Divide the elements of one vector by another ignoring zero

    Division by zero will simply replace the value by zero.

    :param a: the numerator
    :param b: the denominator
    :returns: a / b with infs replaced with zero
    """
    return np.divide(a, b, out=np.zeros_like(b), where=b != 0)


def round_nearest(vec, reference):
    """Round the elements of a vector to the nearst multiple of another vector

    :param vec: the vector to round
    :param reference: the vector to use to find the nearest multiple of
    :returns: the vector rounded to the nearest multiple of thre reference vector
    """
    return np.round(safe_divide(vec, reference)) * reference


def sort_vectors_by_norm(vecs):
    """Sort a list of row vectors by the Euclidean norm
    :param vecs: vectors to sort according to their norms
    :returns: ndarray of sorted row vectors
    """
    idx = np.argsort(norm_along_axis(np.abs(vecs)))
    vecs = vecs[idx]
    return vecs


def norm_along_axis(vecs, axis=-1):
    """Compute the euclidean norm along the rows of a matrix

    :param vecs: vectors to compute the Euclidean norm for
    :param axis: the axis to compute the norm along
    :return: a ndarray with the norms along the chosen axis
    """
    return np.sum(vecs**2, axis=axis)**(1./2)
