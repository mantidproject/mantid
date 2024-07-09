# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np
from numpy.testing import assert_allclose

from abins.kpointsdata import KpointsData, KpointData
from abins.test_helpers import assert_kpoint_almost_equal


class KpointsDataTest(unittest.TestCase):
    _good_data_1 = {
        "k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
        "weights": np.asarray([0.3, 0.2, 0.5]),
        "frequencies": np.asarray(
            [[1.0, 2.0, 34.0, 4.9, 1.0, 2.0], [11.0, 12.0, 134.0, 14.9, 11.0, 12.0], [1.0, 2.0, 34.0, 4.9, 1.0, 2.0]]
        ),  # 6 frequencies for one k-point
        "atomic_displacements": np.asarray(
            [
                [
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 111.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
                ],
                [
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 221.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
                ],
                [
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 31.0], [1.0, 1.0, 1.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0]],
                ],
                # 12 atomic displacements for each k-point
            ]
        ).astype(complex),
        "unit_cell": np.asarray([[7.44, 0.0, 0.0], [0.0, 9.55, 0.0], [0.0, 0.0, 6.92]]),
    }

    # data with soft phonons
    _good_data_2 = {
        "k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
        "weights": np.asarray([0.3, 0.2, 0.5]),
        "frequencies": np.asarray(
            [[-10.0, -2.0, -3.0, 4.9, 1.0, 2.0], [11.0, 12.0, 134.0, 14.9, 11.0, 12.0], [1.0, 2.0, 34.0, 4.9, 1.0, 2.0]]
        ),  # 6 frequencies for one k-point
        "atomic_displacements": np.asarray(
            [
                [
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 121.0, 1.0], [1.0, 1.0, 1.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 131.0], [1.0, 1.0, 1.0]],
                ],
                [
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 221.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],
                ],
                [
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 31.0], [1.0, 1.0, 1.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0]],
                ],
                # 12 atomic displacements for each k-point
            ]
        ).astype(complex),
        "unit_cell": np.asarray([[7.44, 0.0, 0.0], [0.0, 9.55, 0.0], [0.0, 0.0, 6.92]]),
    }

    def test_wrong_value(self):
        # All values should be numpy arrays
        items = self._good_data_1.copy()
        items["k_vectors"] = "wrong_value"

        with self.assertRaises(TypeError):
            KpointsData(**items)

    def test_wrong_weight(self):
        # negative weight (weight should be represented as a positive real number)
        items = self._good_data_1.copy()
        items["weights"] = np.asarray([-0.1, 0.3, 0.2])

        with self.assertRaises(ValueError):
            KpointsData(**items)

    def test_wrong_freq(self):
        # frequencies as a string
        wrong_items = self._good_data_1.copy()
        wrong_items["frequencies"] = "Wrong_freq"

        with self.assertRaises(TypeError):
            KpointsData(**wrong_items)

        # complex frequencies
        wrong_items = self._good_data_1.copy()
        wrong_items["frequencies"] = wrong_items["frequencies"].astype(complex)

        with self.assertRaises(ValueError):
            KpointsData(**wrong_items)

        # frequencies as 2D arrays but with a bad shape
        wrong_items = self._good_data_1.copy()
        wrong_items["frequencies"] = np.asarray([[1.0, 2.0, 34.0], [4.9, 1.0, 1.0]])

        with self.assertRaises(ValueError):
            KpointsData(**wrong_items)

    def test_wrong_displacements(self):
        # displacements as a number
        wrong_items = self._good_data_1.copy()
        wrong_items["atomic_displacements"] = 1

        with self.assertRaises(TypeError):
            KpointsData(**wrong_items)

        # wrong size of the second dimension
        wrong_items = self._good_data_1.copy()
        wrong_items["atomic_displacements"] = np.asarray(
            [
                [
                    [[1.0, 1.0, 11.0], [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 11.0], [1.0, 1.0, 11.0]],
                    [[1.0, 1.0, 1.0], [1.0, 1.0, 11.0], [1.0, 1.0, 11.0], [1.0, 1.0, 1.0], [1.0, 1.0, 11.0], [1.0, 1.0, 11.0]],
                ],
                wrong_items["atomic_displacements"][0, 0],
                wrong_items["atomic_displacements"][0, 1],
            ],
            dtype=object,
        )

        with self.assertRaises(ValueError):
            KpointsData(**wrong_items)

        # displacements as numpy arrays with integers
        wrong_items = self._good_data_1.copy()
        wrong_items["atomic_displacements"] = wrong_items["atomic_displacements"].astype(int)

        with self.assertRaises(ValueError):
            KpointsData(**wrong_items)

        # displacements as a 1D array
        wrong_items = self._good_data_1.copy()
        wrong_items["atomic_displacements"] = np.ravel(wrong_items["atomic_displacements"])

        with self.assertRaises(ValueError):
            KpointsData(**wrong_items)

    def test_set_good_case(self):
        self._set_good_case_core(data=self._good_data_1)
        self._set_good_case_core(data=self._good_data_2)

    def _set_good_case_core(self, data):
        kpd = KpointsData(**data)
        collected_data = kpd.extract()

        for k in range(data["frequencies"].shape[0]):
            self.assertEqual(True, np.allclose(data["frequencies"][k], collected_data["frequencies"][str(k)]))
            self.assertEqual(True, np.allclose(data["atomic_displacements"][k], collected_data["atomic_displacements"][str(k)]))
            self.assertEqual(True, np.allclose(data["k_vectors"][k], collected_data["k_vectors"][str(k)]))
            self.assertEqual(data["weights"][k], collected_data["weights"][str(k)])

    def test_len(self):
        kpd = KpointsData(**self._good_data_1)
        self.assertEqual(len(kpd), 3)

    def test_indexing(self):
        data = self._good_data_1
        k_points_data = KpointsData(**data)

        for kpd in (k_points_data, k_points_data[:], list(k_points_data)):
            for k in range(data["frequencies"].shape[0]):
                self.assertIsInstance(kpd[k], KpointData)
                self.assertEqual(True, np.allclose(data["k_vectors"][k], kpd[k].k))
                self.assertEqual(data["weights"][k], kpd[k].weight)
                self.assertEqual(True, np.allclose(data["frequencies"][k], kpd[k].frequencies))
                self.assertEqual(True, np.allclose(data["atomic_displacements"][k], kpd[k].atomic_displacements))

            with self.assertRaises(IndexError):
                kpd[3]

    def test_json_roundtrip(self):
        ref_data = KpointsData(**self._good_data_2)
        roundtrip_data = KpointsData.from_dict(ref_data.to_dict())

        assert_allclose(ref_data.unit_cell, roundtrip_data.unit_cell)

        for ref_kpt, roundtrip_kpt in zip(ref_data, roundtrip_data):
            assert_kpoint_almost_equal(ref_kpt, roundtrip_kpt)


if __name__ == "__main__":
    unittest.main()
