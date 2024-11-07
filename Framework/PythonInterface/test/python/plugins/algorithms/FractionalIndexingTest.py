# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
import numpy.testing as npt

from mantid.geometry import UnitCell
from mantid.simpleapi import CreatePeaksWorkspace, CreateSimulationWorkspace
import fractional_indexing as indexing


class FractionIndexingTests(unittest.TestCase):
    def setUp(self):
        # Need to set the random seed because the scipy kmeans algorithm
        # randomly initilizes the starting centroids. This can lead to a
        # different but equivilent indexing.
        np.random.seed(10)

    def test_find_bases_with_1d_modulation(self):
        qs = np.array(
            [
                [0, 0, 0.13],
            ]
        )

        ndim, bases = indexing.find_bases(qs, tolerance=0.02)

        self.assertEqual(ndim, 1)
        npt.assert_array_equal(bases[0], qs[0])

    def test_find_bases_with_2d_modulation(self):
        qs = np.array(
            [
                [0, 0, 0.1],
                [0, 0.1, 0],
            ]
        )

        ndim, bases = indexing.find_bases(qs, tolerance=0.02)

        self.assertEqual(ndim, 2)
        npt.assert_array_equal(bases[0], qs[0])
        npt.assert_array_equal(bases[1], qs[1])

    def test_find_bases_with_3d_modulation(self):
        qs = np.array(
            [
                [0, 0, 0.1],
                [0, 0.1, 0],
                [0.1, 0, 0],
            ]
        )

        ndim, bases = indexing.find_bases(qs, tolerance=0.02)

        self.assertEqual(ndim, 3)
        npt.assert_array_equal(bases[0], qs[0])
        npt.assert_array_equal(bases[1], qs[1])
        npt.assert_array_equal(bases[2], qs[2])

    def test_find_bases_with_4d_modulation(self):
        qs = np.array(
            [
                [0, 0, 0.1],
                [0, 0.1, 0],
                [0.1, 0, 0],
                [0.15, 0, 0],
            ]
        )

        ndim, bases = indexing.find_bases(qs, tolerance=0.02)

        self.assertEqual(ndim, 4)
        npt.assert_array_equal(bases[0], qs[0])
        npt.assert_array_equal(bases[1], qs[1])
        npt.assert_array_equal(bases[2], qs[2])

    def test_find_bases_with_2d_modulation_with_linear_combination(self):
        qs = np.array(
            [
                [0, 0.1, 0.1],
                [0, 0, 0.1],
                [0, 0.1, 0],
                [0, 0, 0.15],
                [0, 0.1, 0.2],
            ]
        )

        ndim, bases = indexing.find_bases(qs, tolerance=0.02)

        self.assertEqual(ndim, 3)
        npt.assert_array_equal(bases[0], np.array([0, 0, 0.1]))
        npt.assert_array_equal(bases[1], np.array([0, 0.1, 0]))
        npt.assert_array_equal(bases[2], np.array([0, 0, 0.15]))

    def test_find_bases_with_non_orthogonal_cell(self):
        cell = UnitCell(1, 1, 1, 90, 95, 103)
        cart_cell = cell.getB()
        qs = np.array(
            [
                [0.5, 0, 0],
                [0, 0.5, 0],
                [0, 0, 0.5],
                [0, 0, 0.25],
            ]
        )

        qs = np.dot(qs, cart_cell)

        ndim, bases = indexing.find_bases(qs, 1e-5)
        self.assertEqual(ndim, 3, "Number of dimensions must be 3")

        expected_bases = np.array([[0.0, 0.0, 0.25], [0.0, 0.5, 0.0], [0.51521732, 0.11589868, 0.04490415]])

        npt.assert_almost_equal(bases, expected_bases, err_msg="Basis vectors do not match")

    def test_find_indexing_with_non_orthogonal_cell(self):
        cell = UnitCell(1, 1, 1, 90, 95, 103)
        cart_cell = cell.getB()
        qs = np.array(
            [
                [0.5, 0, 0],
                [0, 0.5, 0],
                [0, 0, 0.5],
                [0, 0, 0.25],
            ]
        )

        qs = np.dot(qs, cart_cell)

        indices = indexing.index_q_vectors(qs)

        expected_indexing = np.array([[0, 0, 1], [0, 1, 0], [2, 0, 0], [1, 0, 0]])

        npt.assert_equal(indices, expected_indexing, err_msg="Indexing does not match expected.")

    def test_index_with_modulation(self):
        qs = np.array(
            [
                [0, 0.1, 0.1],
                [0, -0.1, 0],
                [0, 0, 0.1],
                [0, 0, -0.1],
                [0, 0.1, 0.2],
                [0, 0, 0.45],
            ]
        )

        expected_indexing = np.array([[-1, 1, 0], [1, 0, 0], [0, 1, 0], [0, -1, 0], [-1, 2, 0], [0, 0, 1]])

        actual_indexing = indexing.index_q_vectors(qs, tolerance=0.03)
        npt.assert_array_equal(actual_indexing, expected_indexing)

    def test_norm_along_axis(self):
        vecs = np.array(
            [
                [0, 0, 3],
                [0, 2, 0],
                [1, 0, 0],
                [0, 5, 5],
                [0, 4, 4],
            ]
        )

        expected_output = np.array([3, 2, 1, 7.071068, 5.656854])
        norms = indexing.norm_along_axis(vecs)
        npt.assert_allclose(norms, expected_output)

    def test_sort_vectors_by_norm(self):
        vecs = np.array(
            [
                [0, 0, 3],
                [0, 2, 0],
                [1, 0, 0],
                [0, 5, 5],
                [0, 4, 4],
            ]
        )

        expected_output = np.array(
            [
                [1, 0, 0],
                [0, 2, 0],
                [0, 0, 3],
                [0, 4, 4],
                [0, 5, 5],
            ]
        )

        vecs_sorted = indexing.sort_vectors_by_norm(vecs)
        npt.assert_allclose(vecs_sorted, expected_output)

    def test_trunc_decimals(self):
        reference = np.array([0, 0, 0.1])
        test_input = reference + np.random.random(3) * 0.1
        result = indexing.trunc_decimals(test_input, 1)
        npt.assert_array_equal(result, reference)

    def test_remove_noninteger(self):
        test_input = np.array([1, 2, 3, 1.1, 3.4, -10, -1.8])
        reference = np.array([1, 2, 3, 0, 0, -10, 0])
        result = indexing.remove_noninteger(test_input)
        npt.assert_array_equal(result, reference)

    def test_get_hkls(self):
        ws = CreateSimulationWorkspace("IRIS", BinParams="1,5,10")
        peaks = CreatePeaksWorkspace(ws, 2)
        reference = np.array(
            [
                [1, 1, 2],
                [2, 1, 4],
            ]
        )

        peak = peaks.getPeak(0)
        peak.setHKL(1, 1, 2)
        peak = peaks.getPeak(1)
        peak.setHKL(2, 1, 4)

        hkl = indexing.get_hkls(peaks)
        npt.assert_array_equal(hkl, reference)

    def test_cluster_qs_with_fixed_k(self):
        qs = np.array(
            [
                [0, 0.1, 0.1],
                [0, 0.1, 0.1],
                [0, 0.0, 0.1],
                [0, 0.0, 0.1],
                [0, 0.1, 0.1],
            ]
        )

        qs += np.random.random(qs.shape) * 0.01

        k = 2
        clusters, k = indexing.cluster_qs(qs, k)
        self.assertEqual(k, 2)
        npt.assert_array_equal(clusters, np.array([0, 0, 1, 1, 0]))

    def test_cluster_qs_with_auto_k(self):
        qs = np.array(
            [
                [0, 0.1, 0.1],
                [0, 0.1, 0.1],
                [0, 0.0, 0.1],
                [0, 0.0, 0.1],
                [0, 0.1, 0.1],
            ]
        )

        qs += np.random.random(qs.shape) * 0.01

        clusters, k = indexing.cluster_qs(qs, threshold=0.01)
        self.assertEqual(k, 2)
        npt.assert_array_equal(clusters, np.array([2, 2, 1, 1, 2]))


if __name__ == "__main__":
    unittest.main()
