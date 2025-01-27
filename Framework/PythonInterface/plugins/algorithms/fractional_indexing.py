# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import itertools
import numpy as np
import scipy.cluster.hierarchy as hcluster
from scipy.cluster.vq import kmeans2
from scipy.spatial import KDTree

_MAX_REFLECTIONS = 10


def find_bases(qs, tolerance):
    """Find bases for the higher dimensional indexing scheme

    This will attempt to find a set of basis vectors for the
    higher dimensional indexing scheme.

    :param qs: list of q vectors in HKL space defining satellite peak offsets
    :param tolerance: the tolerance how close each vector must be to be indexed
                      using the generated scheme
    :return: tuple of (number of dimensions, nx3 matrix of basis vectors)
    """
    qs = list(sort_vectors_by_norm(qs))
    final_qs = [qs.pop(0)]
    refs, hkls = generate_hkl_grid(np.array(final_qs), _MAX_REFLECTIONS)

    for q in qs:
        regenerate_grid = not is_indexed(q, refs, tolerance)

        if regenerate_grid:
            final_qs.append(q)
            refs, hkls = generate_hkl_grid(np.array(final_qs), _MAX_REFLECTIONS)

    return len(final_qs), np.vstack(final_qs)


def generate_hkl_grid(bases, upper_bound):
    """Generate a grid of reflections and hkl values

    :param bases: the set of bases to generate the reflections and HKL grid.
    :param upper_bound: the upper bound HKL index to generate
    :return tuple of (nx3 matrix of reflections, nx3 matrix of HKL indices)
    """
    search_range = np.arange(-upper_bound, upper_bound)
    ndim = len(bases)
    hkls = np.array(list(itertools.product(*[search_range] * ndim)))
    reflections = np.array([np.dot(bases.T, hkl) for hkl in hkls])
    return reflections, hkls


def find_nearest_hkl(qs, reflections, hkls, tolerance):
    """Find the nearest HKL value that indexes each of the q vectors

    :param qs: the q vectors for satellite peaks to index
    :param reflections: the list of reflections to search
    :param hkls: the list of hkl values mapping to each reflection
    :param tolerance: the tolerance on the difference between a given q and
                      the nearest reflection
    :return: MxN matrix of integers indexing the q vectors. Unindexed vectors
             will be returned as all zeros.
    """
    kdtree = KDTree(reflections)

    final_indexing = []
    for i, q in enumerate(qs):
        result = kdtree.query_ball_point(q, r=tolerance)
        if len(result) > 0:
            smallest = sort_vectors_by_norm(hkls[result])
            final_indexing.append(smallest[0])
        else:
            final_indexing.append(np.zeros_like(hkls.shape[1]))

    return np.vstack(final_indexing)


def is_indexed(q, reflections, tolerance):
    """Check if a q vector is indexed by a set of reflections

    :param q: the q vector to check
    :param reflections: the list of reflections to find if this matches
    :param tolerance: the tolerance on the difference between q and the nearest
                      reflection.
    :return: whether this q is indexed by this list of reflections
    """
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
    unique_a = np.unique(a.view([("", a.dtype)] * a.shape[1]))
    return unique_a.view(a.dtype).reshape((unique_a.shape[0], a.shape[1]))


def index_q_vectors(qs, tolerance=0.03):
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
        centroids = kmeans_plus_plus(qs, k)
        _, clusters = kmeans2(qs, centroids, minit="matrix")
        if len(set(clusters)) != k:
            raise ValueError(
                "Could not group the satellite reflections into {} clusters. Please check that you have at least {} satellites.".format(
                    k, k
                )
            )
    else:
        clusters = hcluster.fclusterdata(qs, threshold, criterion="distance")
    return clusters, len(set(clusters))


def find_q_vectors(nuclear_hkls, sats_hkls):
    """Find the q vector between the nuclear HKLs and the satellite peaks

    Given a list of HKL positions and a list of fractional HKL positions of
    satellite peaks, find the difference between each satellite and its nearest
    integer HKL.

    :param nuclear_hkls: the positions of integer HKL peaks.
    :param sats_hkls: the positions of fractional "satellite" HKL peaks.
    :returns: np.ndarray -- array of q vectors.
    """
    peak_map = KDTree(nuclear_hkls)
    qs = []
    for sat in sats_hkls:
        distance, index = peak_map.query(sat, k=1)
        if distance > 2:
            # peak too far away from satellite ignore
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
    return np.array([np.array([peak.getH(), peak.getK(), peak.getL()]) for peak in peaks_workspace])


def remove_noninteger(matrix):
    """Remove any non integer values from a matrix

    :param matrix: the matrix to remove non integer values from
    :return: matrix with non integer elements set to zero
    """
    matrix[np.absolute(np.mod(matrix, 1)) > 1e-14] = 0
    return matrix


def trunc_decimals(vec, n_decimals=2):
    """Truncate the elements of a vector below n decimals places

    :param vec: the vector to truncate the elements of
    :param n_decimals: the number of decimal places to truncate from
    :return: the vector with truncated elements
    """
    decade = 10**n_decimals
    return np.trunc(vec * decade) / decade


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
    return np.sum(vecs**2, axis=axis) ** (1.0 / 2)


def kmeans_plus_plus(points, k):
    """Generate centroids for kmeans using the kmeans++ algorithm

    Select a good set of starting values for kmeans by weighting the choice of
    the next centroid by the distance to the existing centroids.

    This algorithm roughly works as follows:
        1) The first centroid is selected uniformly from the data.
        2) The squared euclidean distance between all points to all centroids is calculated.
        3) Take the minimum computed distance for each point over all centroids.
        4) Normalise the list of distances
        5) Choose a new centroid weighted by the computed distances

    :param points: the points to select centroids from
    :param k: the desired number of centroids
    :return: Kx3 matrix of centroids
    """
    if points.shape[0] < k:
        raise RuntimeError("k is greater than the number of points! Please choose a smaller k value")

    centroid_indices = []
    centroids = []

    # pick the first point uniformly at random from the data
    indices = np.indices(points.shape)[0][:, 0]
    centroid_index = np.random.choice(indices)
    centroid = points[centroid_index]

    centroid_indices.append(centroid_index)
    centroids.append(centroid)

    for i in range(k - 1):
        # choose all points that are not already centroids
        mask = np.array([x for x in indices if x not in centroid_indices])
        pts = points[mask]

        # calculate probability distribution for being picked based on squared
        # distance from each centroid
        distance_squared = np.array([norm_along_axis(pts - c, axis=1) ** 2 for c in centroids])
        distance_squared = np.min(distance_squared, axis=0)
        distance_squared /= np.sum(distance_squared)

        # choose a new random centroid weighted by how far it is from the other
        # centroids
        centroid_index = np.random.choice(mask, p=distance_squared)
        centroid_index = indices[centroid_index]
        centroid = points[centroid_index]

        centroid_indices.append(centroid_index)
        centroids.append(centroid)

    return np.array(centroids)
