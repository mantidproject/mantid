# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from abins import KpointsData
from abins.constants import ACOUSTIC_PHONON_THRESHOLD


class KpointsDataTest(unittest.TestCase):

    _good_data_1 = {"k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
                    "weights": np.asarray([0.3, 0.2, 0.5]),
                    "frequencies": np.asarray([[1.0, 2.0, 34.0, 4.9, 1.0, 2.0],
                                               [11.0, 12.0, 134.0, 14.9, 11.0, 12.0],
                                               [1.0, 2.0, 34.0, 4.9, 1.0, 2.0]]),  # 6 frequencies for one k-point
                    "atomic_displacements": np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                        [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],

                                                       [[1.0, 1.0, 1.0], [1.0, 1.0, 111.0], [1.0, 1.0, 1.0],
                                                       [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]],

                                                     [[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                       [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],

                                                      [[1.0, 1.0, 1.0], [1.0, 1.0, 221.0], [1.0, 1.0, 1.0],
                                                      [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]],

                                                     [[[1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0],
                                                      [1.0, 1.0, 1.0], [1.0, 1.0, 31.0], [1.0, 1.0, 1.0]],

                                                      [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                      [1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0]]]
                                                     # 12 atomic displacements for each k-point
                                                      ]).astype(complex),
                    "unit_cell": np.asarray([[ 7.44,  0.  ,  0.  ],
                                             [ 0.  ,  9.55,  0.  ],
                                             [ 0.  ,  0.  ,  6.92]])}

    # data with soft phonons
    _good_data_2 = {"k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
                    "weights": np.asarray([0.3, 0.2, 0.5]),
                    "frequencies": np.asarray([[-10.0, -2.0, -3.0, 4.9, 1.0, 2.0],
                                               [11.0, 12.0, 134.0, 14.9, 11.0, 12.0],
                                               [1.0, 2.0, 34.0, 4.9, 1.0, 2.0]]),  # 6 frequencies for one k-point
                    "atomic_displacements": np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 121.0, 1.0], [1.0, 1.0, 1.0]],

                                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 131.0], [1.0, 1.0, 1.0]]],


                                                        [[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],

                                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 221.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]],


                                                        [[[1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 31.0], [1.0, 1.0, 1.0]],

                                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0]]]
                                                        # 12 atomic displacements for each k-point
                                                        ]).astype(complex),
                    "unit_cell": np.asarray([[7.44, 0., 0.],
                                             [0., 9.55, 0.],
                                             [0., 0., 6.92]])
                    }

    def test_bad_items(self):
        # Dict has wrong contents
        with self.assertRaises(ValueError):
            wrong_dict = {"k_vectors": [], "freq": []}
            KpointsData(items=wrong_dict)

        with self.assertRaises(TypeError):
            wrong_dict= ["k_vectors", 3, "freq"]
            KpointsData(items=wrong_dict)

    def test_missing_key(self):
        # missing atomic_displacements
        items = {"k_vectors": self._good_data_1["k_vectors"],
                 "weights": self._good_data_1["weights"],
                 "frequencies": self._good_data_1["frequencies"]}
        with self.assertRaises(ValueError):
            KpointsData(items=items)

    def test_wrong_value(self):
        # value should be a numpy array with real numbers
        items = {"k_vectors": "wrong_value",
                 "weights": self._good_data_1["weights"],
                 "frequencies": self._good_data_1["frequencies"],
                 "atomic_displacements":  self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            KpointsData(items=items)

    def test_wrong_weight(self):
        # negative weight (weight should be represented as a positive real number)
        items = {"k_vectors": self._good_data_1["k_vectors"],
                 "weights": np.asarray([-0.1, 0.3, 0.2]),
                 "frequencies": self._good_data_1["frequencies"],
                 "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            KpointsData(items=items)

    def test_wrong_freq(self):
        # frequencies as a string
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": "Wrong_freq",
                       "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

        # complex frequencies
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"].astype(complex),
                       "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

        # frequencies as 2D arrays but with a bad shape
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": np.asarray([[1.0, 2.0, 34.0], [4.9, 1.0, 1.0]]),
                       "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

    def test_wrong_displacements(self):
        # displacements as a number
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": 1}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

        # wrong size of the second dimension
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": np.asarray([[[[1., 1., 11.],  [1.,  1., 1., 1.0], [1.0, 1.0, 1.0],
                                                             [1., 1.0, 1.0], [1., 1., 11.],     [1., 1.,  11.]],

                                                            [[1., 1.0, 1.0], [1., 1., 11.],     [1., 1.,  11.],
                                                            [1., 1.0, 1.0],  [1., 1., 11.],     [1., 1.,  11.]]],
                                                           self._good_data_1["atomic_displacements"][0, 0],
                                                           self._good_data_1["atomic_displacements"][0, 1]]
                                                          )}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

        # displacements as numpy arrays with integers
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": self._good_data_1["atomic_displacements"].astype(int)}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

        # displacements as a 1D array
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": np.ravel(self._good_data_1["atomic_displacements"])}
        with self.assertRaises(ValueError):
            KpointsData(items=wrong_items)

    def test_set_good_case(self):

        self._set_good_case_core(data=self._good_data_1)
        self._set_good_case_core(data=self._good_data_2)

    def _set_good_case_core(self, data):
        kpd = KpointsData(items=data)
        collected_data = kpd.extract()

        for k in range(data["frequencies"].shape[0]):
            indices = data["frequencies"][k] > ACOUSTIC_PHONON_THRESHOLD
            temp_f = data["frequencies"][k]
            self.assertEqual(True, np.allclose(temp_f[indices],
                                               collected_data["frequencies"][str(k)]))
            temp_a = data["atomic_displacements"][k]
            self.assertEqual(True, np.allclose(temp_a[:, indices],
                                               collected_data["atomic_displacements"][str(k)]))
            self.assertEqual(True, np.allclose(data["k_vectors"][k], collected_data["k_vectors"][str(k)]))
            self.assertEqual(data["weights"][k], collected_data["weights"][str(k)])


if __name__ == "__main__":
    unittest.main()
