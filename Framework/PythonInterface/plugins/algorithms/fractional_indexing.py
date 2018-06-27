import itertools
import numpy as np
import scipy.cluster.hierarchy as hcluster
from scipy.cluster.vq import kmeans2
from scipy.spatial import KDTree

_MAX_REFLECTIONS = 10


def find_bases(qs, tolerance):
    qs = list(sort_vectors_by_norm(qs))
    regenerate_grid = False
    final_qs = [qs.pop(0)]
    refs, hkls = generate_hkl_grid(np.array(final_qs), _MAX_REFLECTIONS)

    for q in qs:
        regenerate_grid = not is_indexed(q, refs, tolerance)

        if regenerate_grid:
            final_qs.append(q)
            refs, hkls = generate_hkl_grid(np.array(final_qs), _MAX_REFLECTIONS)

    return len(final_qs), np.vstack(final_qs)


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

    return unique_rows(vs)


def generate_hkl_grid(bases, upper_bound):
    search_range = np.arange(-upper_bound, upper_bound)
    ndim = len(bases)
    hkls = np.array(list(itertools.product(*[search_range] * ndim)))
    reflections = np.array([np.dot(bases.T, hkl) for hkl in hkls])
    return reflections, hkls


def find_nearest_hkl(qs, reflections, hkls, tolerance):
    kdtree = KDTree(reflections)

    final_indexing = []
    for i, q in enumerate(qs):
        result = kdtree.query_ball_point(q, r=tolerance)
        if len(result) > 0:
            smallest = sort_vectors_by_norm(hkls[result])
            final_indexing.append(smallest[0])

    return np.vstack(final_indexing)


def is_indexed(q, reflections, tolerance):
    kdtree = KDTree(reflections)
    result = kdtree.query_ball_point(q, r=tolerance)
    return len(result) > 0


def unique_rows(a):
    """Return the unique rows of a numpy array

    In numpy >= 1.13 np.unique(x, axis=0) can be user instead. But for versions of
    numpy before 1.13 this function can be used to achieve the same result.

    :param a: the array to find unique rows for.
    :returns: the unique rows of a
    """
    a = np.ascontiguousarray(a)
    unique_a = np.unique(a.view([('', a.dtype)] * a.shape[1]))
    return unique_a.view(a.dtype).reshape((unique_a.shape[0], a.shape[1]))


def round_to_nearest_reflection(qs, reflections, tolerance):
    """Round the each raw q to the nearest predicted reflection

    :param qs: the list of raw noisy q vectors
    :param reflections: a list of predicted reflection locations.
    :param tolerance: the radius to search for neighbouring reflections in.
    :returns: ndarray of q values rounded to the nearest predicted reflection
    """
    kdtree = KDTree(reflections)

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

    ndim, bases = find_bases(qs, tolerance)
    refs, hkls = generate_hkl_grid(bases, _MAX_REFLECTIONS)
    return find_nearest_hkl(qs, refs, hkls, tolerance)


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


def find_q_vectors(nuclear_hkls, sats_hkls):
    """Find the q vector between the nuclear HKLs and the satellite peaks

    Given a list of HKL positions and a list of fractional HKL positions of
    satellite peaks, find the difference between each satellite and its nearest
    integer HKL.

    :param nuclear_hkls: the positions of integer HKL peaks.
    :param sats_hkl: the positions of fractional "satellite" HKL peaks.
    :returns: np.ndarray -- array of q vectors.
    """
    peak_map = KDTree(nuclear_hkls)
    qs = []
    for sat in sats_hkls:
        distance, index = peak_map.query(sat, k=1)
        if distance > 2:
            # peak to far away from satellite ignore
            continue
        nearest_peak = nuclear_hkls[index]
        qs.append(sat - nearest_peak)
    return np.array(qs)


def find_nearest_integer_peaks(nuclear_hkls, sat_hkls):
    """Find the nearest integer peak for each fractional peak

    This will perform a spatial search to find the intger peak which is nearest the fractional satellite peak.

    :param nuclear_hkls: the hkl poistions of each of the nuclear peaks.
    :param sat_hkls: the fractional hkl position of the satellite peaks.
    :returns: np.ndarray -- 2D array of HKL integer values for each fractional peak
    """
    peak_map = KDTree(nuclear_hkls)
    hkls = []
    for sat in sat_hkls:
        distance, index = peak_map.query(sat, k=1)
        nearest_peak = nuclear_hkls[index]
        hkls.append(nearest_peak)
    return np.array(hkls)


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
