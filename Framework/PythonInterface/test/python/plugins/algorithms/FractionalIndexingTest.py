import unittest
import numpy as np
import numpy.testing as npt
import fractional_indexing as indexing

from mantid.kernel import V3D
from mantid.simpleapi import CreatePeaksWorkspace, CreateSimulationWorkspace


class FractionIndexingTests(unittest.TestCase):

    def setUp(self):
        # Need to set the random seed because the scipy kmeans algorithm
        # randomly initilizes the starting centroids. This can lead to a
        # different but equivilent indexing.
        np.random.seed(10)

    def test_find_bases_with_1d_modulation(self):
        qs = np.array([
            [0, 0, .13],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 1)
        npt.assert_array_equal(bases[1], qs[0])

    def test_find_bases_with_2d_modulation(self):
        qs = np.array([
            [0, 0, .1],
            [0, .1, 0],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        print bases

        self.assertEqual(ndim, 2)
        npt.assert_array_equal(bases[1], qs[0])
        npt.assert_array_equal(bases[2], qs[1])

    def test_find_bases_with_3d_modulation(self):
        qs = np.array([
            [0, 0, .1],
            [0, .1, 0],
            [.1, 0, 0],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 3)
        npt.assert_array_equal(bases[1], qs[0])
        npt.assert_array_equal(bases[2], qs[1])
        npt.assert_array_equal(bases[3], qs[2])

    def test_find_bases_with_4d_modulation(self):
        qs = np.array([
            [0, 0, .1],
            [0, .1, 0],
            [.1, 0, 0],
            [.15, 0, 0],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 4)
        npt.assert_array_equal(bases[1], qs[0])
        npt.assert_array_equal(bases[2], qs[1])
        npt.assert_array_equal(bases[3], qs[2])

    def test_find_bases_with_2d_modulation_with_linear_combination(self):
        qs = np.array([
            [0, .1, .1],
            [0, 0, .1],
            [0, .1, 0],
            [0, 0, .15],
            [0, .1, .2],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 3)
        npt.assert_array_equal(bases[1], np.array([0, .1, 0]))
        npt.assert_array_equal(bases[2], np.array([0, 0, .1]))
        npt.assert_array_equal(bases[3], np.array([0, 0, .15]))

    def test_generate_grid(self):
        bases = np.array([
            [0, .1, 0],
            [0, 0, .1],
            [0, 0, .14],
        ])
        expected_result = np.array([
            [-0., -0.2, -0.28],
            [-0., -0.2, -0.2],
            [-0., -0.2, -0.14],
            [-0., -0.2, -0.1],
            [0., -0.2,  0.],
            [0., -0.2,  0.1],
            [0., -0.2,  0.14],
            [-0., -0.1, -0.28],
            [-0., -0.1, -0.2],
            [-0., -0.1, -0.14],
            [-0., -0.1, -0.1],
            [0., -0.1,  0.],
            [0., -0.1,  0.1],
            [0., -0.1,  0.14],
            [0.,  0., -0.28],
            [0.,  0., -0.2],
            [0.,  0., -0.14],
            [0.,  0., -0.1],
            [0.,  0.,  0.],
            [0.,  0.,  0.1],
            [0.,  0.,  0.14],
            [0.,  0.1, -0.28],
            [0.,  0.1, -0.2],
            [0.,  0.1, -0.14],
            [0.,  0.1, -0.1],
            [0.,  0.1,  0.],
            [0.,  0.1,  0.1],
            [0.,  0.1,  0.14]
        ])

        result = indexing.generate_grid(bases, 2)
        npt.assert_array_equal(result, expected_result)

    def test_index_with_2d_moduldation_and_noise(self):
        qs = np.array([
            [0, .1, .1],
            [0, -.1, 0],
            [0, 0, .11],
            [0, 0, -.14],
            [0, 0, .17],
            [0, .1, .2],
        ])

        expected_indexing = np.array([
            [1, 1, 0],
            [-1, 0, 0],
            [0, 1, 0],
            [0, 0, 1],
            [0, 0, -1],
            [1, 2, 0]
        ])

        actual_indexing = indexing.index_q_vectors(qs, tolerance=.04)
        npt.assert_array_equal(actual_indexing, expected_indexing)

    def test_norm_along_axis(self):
        vecs = np.array([
            [0, 0, 3],
            [0, 2, 0],
            [1, 0, 0],
            [0, 5, 5],
            [0, 4, 4],
        ])

        expected_output = np.array([3, 2, 1, 7.071068, 5.656854])
        norms = indexing.norm_along_axis(vecs)
        npt.assert_allclose(norms, expected_output)

    def test_sort_vectors_by_norm(self):
        vecs = np.array([
            [0, 0, 3],
            [0, 2, 0],
            [1, 0, 0],
            [0, 5, 5],
            [0, 4, 4],
        ])

        expected_output = np.array([
            [1, 0, 0],
            [0, 2, 0],
            [0, 0, 3],
            [0, 4, 4],
            [0, 5, 5],
        ])

        vecs_sorted = indexing.sort_vectors_by_norm(vecs)
        npt.assert_allclose(vecs_sorted, expected_output)

    def test_round_nearest(self):
        expected = np.array([
            [0, 0, .2],
            [0, 0, .1],
            [0, 0, 0],
            [0, 0, -.2],
            [0, 0, -.1],
            [.1, .1, .2],
            [-.1, -.1, -.2],
        ])

        vecs = np.array([
            [0, 0, .21],
            [0, 0, .09],
            [0, 0, 0],
            [0, 0, -.21],
            [0, 0, -.09],
            [.11, .11, .21],
            [-.11, -.11, -.21],
        ])

        for test_vec, reference in zip(vecs, expected):
            result = indexing.round_nearest(test_vec, np.array([.1, .1, .1]))
            npt.assert_array_equal(result, reference)

    def test_safe_divide(self):
        reference = np.array([0, 0, 4])
        result = indexing.safe_divide(np.array([0,0,8]), np.array([0,0,2]))
        npt.assert_array_equal(result, reference)

    def test_safe_divide_zero_vector(self):
        reference = np.array([0, 0, 0])
        result = indexing.safe_divide(np.array([0,0,8]), np.array([0,0,0]))
        npt.assert_array_equal(result, reference)

    def test_trunc_decimals(self):
        reference = np.array([0, 0, 0.1])
        test_input = reference + np.random.random(3) * 0.1
        result = indexing.trunc_decimals(test_input, 1)
        npt.assert_array_equal(result, reference)

    def test_remove_nonzero(self):
        test_input = np.array([1, 2, 3, 1.1, 3.4, -10, -1.8])
        reference = np.array([1, 2, 3, 0, 0, -10, 0])
        result = indexing.remove_nonzero(test_input)
        npt.assert_array_equal(result, reference)

    def test_get_hkls(self):
        ws = CreateSimulationWorkspace("IRIS", BinParams="1,5,10")
        peaks = CreatePeaksWorkspace(ws, 2)
        reference = np.array([
            [1, 1, 2],
            [2, 1, 4],
        ])

        peak = peaks.getPeak(0)
        peak.setHKL(1, 1, 2)
        peak = peaks.getPeak(1)
        peak.setHKL(2, 1, 4)

        hkl = indexing.get_hkls(peaks)
        npt.assert_array_equal(hkl, reference)

    def test_cluster_qs_with_fixed_k(self):
        qs = np.array([
            [0, .1, .1],
            [0, .1, .1],
            [0, .0, .1],
            [0, .0, .1],
            [0, .1, .1],
        ])

        qs += np.random.random(qs.shape) * 0.01

        k = 2
        clusters, k = indexing.cluster_qs(qs, k)
        self.assertEqual(k, 2)
        npt.assert_array_equal(clusters, np.array([0, 0, 1, 1, 0]))

    def test_cluster_qs_with_auto_k(self):
        qs = np.array([
            [0, .1, .1],
            [0, .1, .1],
            [0, .0, .1],
            [0, .0, .1],
            [0, .1, .1],
        ])

        qs += np.random.random(qs.shape) * 0.01

        clusters, k = indexing.cluster_qs(qs, threshold=0.01)
        self.assertEqual(k, 2)
        npt.assert_array_equal(clusters, np.array([2, 2, 1, 1, 2]))

    def test_round_nearest_reflections(self):
        reference = np.array([
            [0, .1, .1],
            [0, .1, .1],
            [0, .0, .1],
            [0, .0, .2],
            [0, .1, .2],
        ])

        qs = reference + np.random.random(reference.shape) * 0.01

        reflections = indexing.generate_grid(np.array([[0, 0, .1], [0, .1, 0]]), 3)
        rounded = indexing.round_to_nearest_reflection(qs, reflections, .03)
        npt.assert_array_equal(rounded, reference)


if __name__ == "__main__":
    unittest.main()
